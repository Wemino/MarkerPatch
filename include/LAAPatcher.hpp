#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace LAAPatcher
{
    constexpr wchar_t RESTART_ENV_VAR[] = L"MARKERPATCH_LAA_RESTART";

    // ========================
    // Error Handling
    // ========================

    inline std::wstring* g_errorOutput = nullptr;

    inline void ShowError(const char* message)
    {
        if (g_errorOutput)
        {
            *g_errorOutput = std::wstring(message, message + strlen(message));
            return;
        }

        MessageBoxA(NULL, message, "MarkerPatch - LAAPatcher Error", MB_ICONERROR);
    }

    inline void ShowError(const wchar_t* message)
    {
        if (g_errorOutput)
        {
            *g_errorOutput = message;
            return;
        }

        MessageBoxW(NULL, message, L"MarkerPatch - LAAPatcher Error", MB_ICONERROR);
    }

    // ========================
    // Validation Helpers
    // ========================

    inline bool CheckBounds(size_t offset, size_t size, size_t bufferSize, const char* context, wchar_t* outError, size_t errorSize)
    {
        if (offset <= bufferSize && size <= bufferSize - offset)
            return true;

        swprintf_s(outError, errorSize, L"%S exceeds bounds (offset=0x%zX, size=0x%zX, bufferSize=%zu).", context, offset, size, bufferSize);
        return false;
    }

    // ========================
    // PE Header Wrapper
    // ========================

    class PEFile
    {
    public:
        explicit PEFile(std::vector<uint8_t>& buffer) : m_buffer(buffer) {}

        bool Validate(wchar_t* outError, size_t errorSize)
        {
            if (m_buffer.size() < sizeof(IMAGE_DOS_HEADER))
            {
                swprintf_s(outError, errorSize, L"File too small (%zu bytes, need at least %zu).", m_buffer.size(), sizeof(IMAGE_DOS_HEADER));
                return false;
            }

            if (DosHeader()->e_magic != IMAGE_DOS_SIGNATURE)
            {
                swprintf_s(outError, errorSize, L"Invalid DOS signature (0x%04X, expected 0x%04X).", DosHeader()->e_magic, IMAGE_DOS_SIGNATURE);
                return false;
            }

            if (DosHeader()->e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)))
            {
                swprintf_s(outError, errorSize, L"Invalid e_lfanew value (%d, must be at least %zu).", DosHeader()->e_lfanew, sizeof(IMAGE_DOS_HEADER));
                return false;
            }

            if (!CheckBounds(DosHeader()->e_lfanew, sizeof(IMAGE_NT_HEADERS), m_buffer.size(), "NT header", outError, errorSize))
                return false;

            if (NtHeaders()->Signature != IMAGE_NT_SIGNATURE)
            {
                swprintf_s(outError, errorSize, L"Invalid NT signature (0x%08X, expected 0x%08X).", NtHeaders()->Signature, IMAGE_NT_SIGNATURE);
                return false;
            }

            if (NtHeaders()->FileHeader.NumberOfSections == 0)
            {
                swprintf_s(outError, errorSize, L"PE file has zero sections.");
                return false;
            }

            size_t sectionHeadersOffset = DosHeader()->e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + NtHeaders()->FileHeader.SizeOfOptionalHeader;
            size_t sectionHeadersSize = NtHeaders()->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

            if (!CheckBounds(sectionHeadersOffset, sectionHeadersSize, m_buffer.size(), "Section headers", outError, errorSize))
                return false;

            return true;
        }

        PIMAGE_DOS_HEADER DosHeader() { return reinterpret_cast<PIMAGE_DOS_HEADER>(m_buffer.data()); }
        PIMAGE_NT_HEADERS NtHeaders() { return reinterpret_cast<PIMAGE_NT_HEADERS>(m_buffer.data() + DosHeader()->e_lfanew); }

        bool IsLAAEnabled() const { return (const_cast<PEFile*>(this)->NtHeaders()->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0; }
        void EnableLAA() { NtHeaders()->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE; }
        void ClearChecksum() { NtHeaders()->OptionalHeader.CheckSum = 0; }

    private:
        std::vector<uint8_t>& m_buffer;
    };

    // ========================
    // File I/O
    // ========================

    inline bool ReadFile(const fs::path& path, std::vector<uint8_t>& buffer)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            return false;

        std::streamsize size = file.tellg();
        if (size <= 0)
            return false;

        file.seekg(0, std::ios::beg);

        try
        {
            buffer.resize(static_cast<size_t>(size));
        }
        catch (...)
        {
            return false;
        }

        return file.read(reinterpret_cast<char*>(buffer.data()), size).good();
    }

    inline bool MoveFileWithRetry(const fs::path& from, const fs::path& to, DWORD flags)
    {
        for (int i = 0; i < 20; i++)
        {
            if (MoveFileExW(from.wstring().c_str(), to.wstring().c_str(), flags))
                return true;

            DWORD err = GetLastError();
            if (err != ERROR_SHARING_VIOLATION && err != ERROR_LOCK_VIOLATION && err != ERROR_ACCESS_DENIED)
                return false;

            Sleep(100);
        }

        return false;
    }

    // ========================
    // Validation
    // ========================

    inline bool ValidatePatchedFile(const fs::path& origPath, const fs::path& patchPath)
    {
        wchar_t error[512];
        std::vector<uint8_t> origData, patchData;

        if (!ReadFile(origPath, origData))
        {
            swprintf_s(error, L"Validation failed: Could not read original file.\nPath: %s", origPath.wstring().c_str());
            ShowError(error);
            return false;
        }

        if (!ReadFile(patchPath, patchData))
        {
            swprintf_s(error, L"Validation failed: Could not read patched file.\nPath: %s", patchPath.wstring().c_str());
            ShowError(error);
            return false;
        }

        PEFile origPE(origData);
        PEFile patchPE(patchData);

        if (!origPE.Validate(error, _countof(error)))
        {
            ShowError(error);
            return false;
        }

        if (!patchPE.Validate(error, _countof(error)))
        {
            ShowError(error);
            return false;
        }

        if (origData.size() != patchData.size())
        {
            swprintf_s(error, L"Validation failed: File size changed (original=%zu, patched=%zu).", origData.size(), patchData.size());
            ShowError(error);
            return false;
        }

        if (!patchPE.IsLAAEnabled())
        {
            ShowError("Validation failed: The LAA flag is not set in the patched file.");
            return false;
        }

        origPE.EnableLAA();
        origPE.ClearChecksum();
        patchPE.ClearChecksum();

        if (memcmp(origData.data(), patchData.data(), origData.size()) != 0)
        {
            ShowError("Validation failed: File differs in more than just LAA flag and checksum.");
            return false;
        }

        return true;
    }

    // ========================
    // File Replacement
    // ========================

    // Writes the patched file next to the executable, validates it, then swaps them (the original is kept as a backup)
    inline bool ReplaceExecutable(const fs::path& exePath, const std::vector<uint8_t>& buffer)
    {
        wchar_t error[512];
        std::error_code ec;

        std::wstring exeName = exePath.filename().wstring();
        fs::path newPath = exePath; newPath += ".new";
        fs::path bakPath = exePath; bakPath += ".bak";

        if (fs::exists(newPath, ec))
        {
            fs::remove(newPath, ec);
            if (ec)
            {
                swprintf_s(error, L"Failed to remove existing .new file.\nPath: %s\nError: %S", newPath.wstring().c_str(), ec.message().c_str());
                ShowError(error);
                return false;
            }
        }

        {
            std::ofstream outFile(newPath, std::ios::binary);
            if (!outFile)
            {
                DWORD err = GetLastError();
                if (err == ERROR_ACCESS_DENIED)
                    swprintf_s(error, L"Unable to write patched file (Access Denied).\n\nYour Anti-Virus may be blocking modification.\nPlease add an exception for this folder.\nPath: %s", newPath.wstring().c_str());
                else
                    swprintf_s(error, L"Failed to create patched file.\nPath: %s\nError: %d", newPath.wstring().c_str(), err);

                ShowError(error);
                return false;
            }

            outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

            if (!outFile.good())
            {
                swprintf_s(error, L"Failed to write patched file data.\nPath: %s", newPath.wstring().c_str());
                ShowError(error);
                outFile.close();
                fs::remove(newPath, ec);
                return false;
            }
        }

        // Make sure the patched file is on disk before replacing the original
        HANDLE hNewFile = CreateFileW(newPath.wstring().c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hNewFile != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(hNewFile);
            CloseHandle(hNewFile);
        }

        auto writtenSize = fs::file_size(newPath, ec);
        if (ec || writtenSize != buffer.size())
        {
            swprintf_s(error, L"Written file size mismatch (expected=%zu, actual=%zu).\nPath: %s", buffer.size(), ec ? 0 : static_cast<size_t>(writtenSize), newPath.wstring().c_str());
            ShowError(error);
            fs::remove(newPath, ec);
            return false;
        }

        if (!ValidatePatchedFile(exePath, newPath))
        {
            fs::remove(newPath, ec);
            return false;
        }

        // Don't overwrite an existing backup unless it's identical to the current executable
        std::vector<uint8_t> bakData, exeData;
        if (fs::exists(bakPath, ec) && !(ReadFile(bakPath, bakData) && ReadFile(exePath, exeData) && bakData == exeData))
        {
            for (int i = 1; fs::exists(bakPath, ec); i++)
            {
                bakPath = exePath;
                bakPath += ".bak" + std::to_string(i);
            }
        }

        if (!MoveFileWithRetry(exePath, bakPath, MOVEFILE_REPLACE_EXISTING))
        {
            DWORD err = GetLastError();

            if (err == ERROR_ACCESS_DENIED)
            {
                swprintf_s(error, L"LAA Patch failed: Access Denied (Error 5).\n\n"
                    L"Windows is blocking the modification of %s.\n"
                    L"Please restart the game as Administrator to apply the patch.\n\n"
                    L"You can disable this check by setting CheckLAAPatch=0 in MarkerPatch.ini", exeName.c_str());
            }
            else if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION)
            {
                swprintf_s(error, L"LAA Patch failed: %s is locked by another process.\n\nError: %d\n\nYou can disable this check by setting CheckLAAPatch=0 in MarkerPatch.ini", exeName.c_str(), err);
            }
            else
            {
                swprintf_s(error, L"Failed to backup %s.\n\nError: %d", exeName.c_str(), err);
            }

            ShowError(error);
            fs::remove(newPath, ec);
            return false;
        }

        if (!MoveFileWithRetry(newPath, exePath, 0))
        {
            DWORD err = GetLastError();

            if (!MoveFileWithRetry(bakPath, exePath, 0))
            {
                swprintf_s(error, L"Failed to rename patched file to %s, and restoring the backup also failed.\n\nError: %d\n\nPlease close the game and rename %s back to %s.", exeName.c_str(), err, bakPath.filename().wstring().c_str(), exeName.c_str());
                ShowError(error);
                return false;
            }

            swprintf_s(error, L"Failed to rename patched file to %s. Restored from backup.\n\nError: %d", exeName.c_str(), err);
            ShowError(error);
            fs::remove(newPath, ec);
            return false;
        }

        return true;
    }

    // ========================
    // Restart Handling
    // ========================

    // Lets the restarted game start only once the previous instance has exited (called from DLL_PROCESS_ATTACH)
    inline void WaitForPreviousInstance()
    {
        wchar_t value[16];
        DWORD len = GetEnvironmentVariableW(RESTART_ENV_VAR, value, 16);
        if (len == 0 || len >= 16)
            return;

        SetEnvironmentVariableW(RESTART_ENV_VAR, NULL);

        DWORD pid = wcstoul(value, NULL, 10);
        HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (hProcess)
        {
            WaitForSingleObject(hProcess, 10000);
            CloseHandle(hProcess);
        }
    }

    // Restart with the same command line, the new instance waits for this one to exit
    inline bool RestartGame(const fs::path& exePath)
    {
        SetEnvironmentVariableW(RESTART_ENV_VAR, std::to_wstring(GetCurrentProcessId()).c_str());

        std::wstring commandLine = GetCommandLineW();
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        if (CreateProcessW(exePath.wstring().c_str(), commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            ExitProcess(0);
        }

        DWORD err = GetLastError();
        SetEnvironmentVariableW(RESTART_ENV_VAR, NULL);
        SetLastError(err);
        return false;
    }

    // ========================
    // Main Patching Logic
    // ========================

    inline bool GetExecutablePath(HMODULE hModule, fs::path& exePath)
    {
        wchar_t error[512];

        if (!hModule)
        {
            ShowError("PerformLAAPatch called with NULL module handle.");
            return false;
        }

        std::wstring modulePathBuf(MAX_PATH, L'\0');
        DWORD pathLen = GetModuleFileNameW(hModule, modulePathBuf.data(), static_cast<DWORD>(modulePathBuf.size()));
        while (pathLen == modulePathBuf.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            modulePathBuf.resize(modulePathBuf.size() * 2);
            pathLen = GetModuleFileNameW(hModule, modulePathBuf.data(), static_cast<DWORD>(modulePathBuf.size()));
        }

        if (pathLen == 0)
        {
            swprintf_s(error, L"GetModuleFileNameW failed. Error: %d", GetLastError());
            ShowError(error);
            return false;
        }

        modulePathBuf.resize(pathLen);

        exePath = modulePathBuf;

        std::error_code ec;
        if (!fs::exists(exePath, ec))
        {
            swprintf_s(error, L"Executable path does not exist: %s", exePath.wstring().c_str());
            ShowError(error);
            return false;
        }

        return true;
    }

    inline bool ReadExecutable(const fs::path& exePath, std::vector<uint8_t>& buffer)
    {
        wchar_t error[512];

        if (!ReadFile(exePath, buffer))
        {
            swprintf_s(error, L"Could not read executable file.\nPath: %s\nError: %d", exePath.wstring().c_str(), GetLastError());
            ShowError(error);
            return false;
        }

        PEFile pe(buffer);
        if (!pe.Validate(error, _countof(error)))
        {
            ShowError(error);
            return false;
        }

        return true;
    }

    inline bool HasSpaceForPatch(const fs::path& exePath, size_t exeSize)
    {
        wchar_t error[512];
        std::error_code ec;

        auto spaceInfo = fs::space(exePath.parent_path(), ec);
        if (!ec)
        {
            uintmax_t requiredSpace = exeSize * 2;
            if (spaceInfo.available < requiredSpace)
            {
                swprintf_s(error, L"Not enough disk space to create backup and patched files.\n\nRequired: %llu MB\nAvailable: %llu MB", (requiredSpace / (1024 * 1024)) + 1, spaceInfo.available / (1024 * 1024));
                ShowError(error);
                return false;
            }
        }

        return true;
    }

    inline bool PatchExecutable(const fs::path& exePath, std::vector<uint8_t>& buffer)
    {
        PEFile pe(buffer);
        pe.EnableLAA();
        pe.ClearChecksum();

        return ReplaceExecutable(exePath, buffer);
    }

    inline bool PerformLAAPatch(HMODULE hModule, bool showConfirmation)
    {
        wchar_t error[512];
        fs::path exePath;
        std::vector<uint8_t> buffer;

        if (!GetExecutablePath(hModule, exePath) || !ReadExecutable(exePath, buffer))
            return false;

        if (PEFile(buffer).IsLAAEnabled())
            return true;

        if (!HasSpaceForPatch(exePath, buffer.size()))
            return false;

        if (showConfirmation)
        {
            std::wstring msg = L"Your game executable is missing the 4GB/LAA patch. "
                L"This allows the game to use 4GB of memory instead of 2GB.\n\n"
                L"MarkerPatch will patch " + exePath.filename().wstring() + L" and create a backup.\n\n"
                L"Apply patch and restart the game?\n\n"
                L"(This check can be disabled by setting CheckLAAPatch=0 in the [General] section of MarkerPatch.ini)";

            if (MessageBoxW(NULL, msg.c_str(), L"4GB/Large Address Aware patch missing!", MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                return false;
        }

        if (!PatchExecutable(exePath, buffer))
            return false;

        if (!RestartGame(exePath))
        {
            swprintf_s(error, L"Patching completed successfully, but failed to restart the application.\nError: %d\n\nPlease restart the game manually.", GetLastError());
            MessageBoxW(NULL, error, L"MarkerPatch - LAAPatcher Warning", MB_ICONWARNING);
        }

        return true;
    }
}