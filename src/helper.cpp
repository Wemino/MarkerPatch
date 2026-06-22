#pragma once

#include "safetyhook/safetyhook.hpp"
#include "dllmain.hpp"

namespace MemoryHelper
{
	template <typename T> static bool WriteMemory(uintptr_t address, T value, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		*reinterpret_cast <T*> (address) = value;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), oldProtect, &oldProtect);
		}
		return true;
	}

	static bool WriteMemoryRaw(uintptr_t address, const void* data, size_t size, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		std::memcpy(reinterpret_cast <void*> (address), data, size);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), size, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeNOP(uintptr_t address, size_t count, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), count, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		std::memset(reinterpret_cast <void*> (address), 0x90, count);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), count, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeCALL(uintptr_t srcAddress, uintptr_t destAddress, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		uintptr_t relativeAddress = destAddress - srcAddress - 5; *reinterpret_cast <uint8_t*> (srcAddress) = 0xE8; // CALL opcode
		*reinterpret_cast <uintptr_t*> (srcAddress + 1) = relativeAddress;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, oldProtect, &oldProtect);
		}
		return true;
	}

	static bool MakeJMP(uintptr_t srcAddress, uintptr_t destAddress, bool disableProtection = true)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
		}
		uintptr_t relativeAddress = destAddress - srcAddress - 5; *reinterpret_cast <uint8_t*> (srcAddress) = 0xE9; // JMP opcode
		*reinterpret_cast <uintptr_t*> (srcAddress + 1) = relativeAddress;
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (srcAddress), 5, oldProtect, &oldProtect);
		}
		return true;
	}

	template <typename T> static T ReadMemory(uintptr_t address, bool disableProtection = false)
	{
		DWORD oldProtect;
		if (disableProtection)
		{
			if (!VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), PAGE_EXECUTE_READ, &oldProtect)) return T();
		}
		T value = *reinterpret_cast <T*> (address);
		if (disableProtection)
		{
			VirtualProtect(reinterpret_cast <LPVOID> (address), sizeof(T), oldProtect, &oldProtect);
		}
		return value;
	}

	DWORD PatternScan(HMODULE hModule, std::string_view signature)
	{
		auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

		auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hModule) + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

		BYTE* base = reinterpret_cast<BYTE*>(hModule);
		DWORD imageSize = ntHeaders->OptionalHeader.SizeOfImage;

		uint8_t pat[256] = {};
		uint8_t msk[256] = {};
		size_t patSize = 0;

		for (size_t i = 0; i < signature.length() && patSize < 256; i++)
		{
			if (signature[i] == ' ') continue;
			if (signature[i] == '?')
			{
				patSize++;
				if (i + 1 < signature.length() && signature[i + 1] == '?') i++;
			}
			else
			{
				auto h = [](char c) -> uint8_t
					{
						if (c >= '0' && c <= '9') return c - '0';
						if (c >= 'A' && c <= 'F') return c - 'A' + 10;
						if (c >= 'a' && c <= 'f') return c - 'a' + 10;
						return 0;
					};

				pat[patSize] = (h(signature[i]) << 4) | h(signature[i + 1]);
				msk[patSize] = 0xFF;
				patSize++;
				i++;
			}
		}

		if (patSize == 0 || imageSize < patSize) return 0;

		size_t a1 = SIZE_MAX, a2 = SIZE_MAX;
		for (size_t i = 0; i < patSize; i++)
		{
			if (msk[i])
			{
				if (a1 == SIZE_MAX) a1 = i;
				a2 = i;
			}
		}

		if (a1 == SIZE_MAX) return reinterpret_cast<DWORD>(base);
		const bool useDual = (a2 != a1);

		__m128i simPat = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pat));
		__m128i simMask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(msk));
		__m128i needle1 = _mm_set1_epi8(static_cast<char>(pat[a1]));
		__m128i needle2 = useDual ? _mm_set1_epi8(static_cast<char>(pat[a2])) : _mm_setzero_si128();

		BYTE* scanEnd = base + imageSize - patSize;

		size_t maxAnchor = useDual ? (a1 > a2 ? a1 : a2) : a1;
		size_t margin = maxAnchor + 16;
		if (margin < patSize + 15) margin = patSize + 15;
		if (margin < 31) margin = 31;

		BYTE* simdEnd = (imageSize > margin) ? base + imageSize - margin : base;
		BYTE* pos = base;

		while (pos <= simdEnd)
		{
			__m128i blk1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pos + a1));
			int hits = _mm_movemask_epi8(_mm_cmpeq_epi8(blk1, needle1));

			if (hits && useDual)
			{
				__m128i blk2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pos + a2));
				hits &= _mm_movemask_epi8(_mm_cmpeq_epi8(blk2, needle2));
			}

			while (hits)
			{
				unsigned long bit;
				_BitScanForward(&bit, hits);
				hits &= hits - 1;

				BYTE* cand = pos + bit;

				__m128i mem = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cand));
				__m128i masked = _mm_and_si128(mem, simMask);

				if (_mm_movemask_epi8(_mm_cmpeq_epi8(masked, simPat)) == 0xFFFF)
				{
					if (patSize <= 16)
						return reinterpret_cast<DWORD>(cand);

					bool ok = true;
					for (size_t k = 16; k < patSize; k++)
					{
						if (msk[k] && cand[k] != pat[k]) { ok = false; break; }
					}

					if (ok) return reinterpret_cast<DWORD>(cand);
				}
			}

			pos += 16;
		}

		while (pos <= scanEnd)
		{
			if (pos[a1] == pat[a1] && (!useDual || pos[a2] == pat[a2]))
			{
				bool ok = true;
				for (size_t k = 0; k < patSize; k++)
				{
					if (msk[k] && pos[k] != pat[k]) { ok = false; break; }
				}

				if (ok) return reinterpret_cast<DWORD>(pos);
			}

			pos++;
		}

		return 0;
	}

	DWORD FindSignatureAddress(HMODULE Module, std::string_view Signature, int FunctionStartCheckCount = -1)
	{
		DWORD Address = static_cast<DWORD>(PatternScan(Module, Signature));
		if (Address == 0) 
			return 0;

		if (FunctionStartCheckCount >= 0)
		{
			// After a RET, compilers pad with INT3 (0xCC) to align the next function (often on a 16-byte boundary).
			// This padding also acts as a breakpoint if execution runs off the end of a function.
			// We backtrack past any 0xCC bytes to find the real function start.
			for (DWORD ScanAddress = Address; ScanAddress > Address - 0x1000; ScanAddress--)
			{
				bool IsValid = true;
				for (int OffsetIndex = 1; OffsetIndex <= FunctionStartCheckCount; OffsetIndex++)
				{
					if (ReadMemory<uint8_t>(ScanAddress - OffsetIndex) != 0xCC)
					{
						IsValid = false;
						break;
					}
				}
				if (IsValid)
					return ScanAddress;
			}
		}

		return Address;
	}

	DWORD ResolveRelativeAddress(uintptr_t BaseAddress, std::size_t InstructionOffset)
	{
		if (BaseAddress == 0) return 0;

		int RelativeOffset = ReadMemory<int>(BaseAddress + InstructionOffset);
		return BaseAddress + InstructionOffset + sizeof(RelativeOffset) + RelativeOffset;
	}
};

namespace HookHelper
{
	static void LogHookError(void* addr, const safetyhook::InlineHook::Error& err)
	{
		char errorMsg[0x200];
		const char* errorType = "Unknown error";

		switch (err.type) 
		{
			case safetyhook::InlineHook::Error::BAD_ALLOCATION:
				errorType = "BAD_ALLOCATION";
				break;
			case safetyhook::InlineHook::Error::FAILED_TO_DECODE_INSTRUCTION:
				errorType = "FAILED_TO_DECODE_INSTRUCTION";
				break;
			case safetyhook::InlineHook::Error::SHORT_JUMP_IN_TRAMPOLINE:
				errorType = "SHORT_JUMP_IN_TRAMPOLINE";
				break;
			case safetyhook::InlineHook::Error::IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE:
				errorType = "IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE";
				break;
			case safetyhook::InlineHook::Error::UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE:
				errorType = "UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE";
				break;
			case safetyhook::InlineHook::Error::FAILED_TO_UNPROTECT:
				errorType = "FAILED_TO_UNPROTECT";
				break;
			case safetyhook::InlineHook::Error::NOT_ENOUGH_SPACE:
				errorType = "NOT_ENOUGH_SPACE";
				break;
		}

		sprintf_s(errorMsg, "Failed to create hook at %p\nError: %s", addr, errorType);
		MessageBoxA(NULL, errorMsg, "SafetyHook Error", MB_ICONERROR | MB_OK);
	}

	static safetyhook::InlineHook CreateHook(void* addr, void* hookFunc)
	{
		auto hook = safetyhook::create_inline(addr, hookFunc);

		if (!hook) 
		{
			auto result = safetyhook::InlineHook::create(addr, hookFunc);
			if (!result) 
			{
				LogHookError(addr, result.error());
			}
		}

		return hook;
	}

	static safetyhook::InlineHook CreateHookAPI(LPCWSTR moduleName, LPCSTR apiName, void* hookFunc)
	{
		HMODULE module = GetModuleHandleW(moduleName);
		if (!module) 
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to get module: %ls", moduleName);
			MessageBoxA(NULL, errorMsg, "SafetyHook Error", MB_ICONERROR | MB_OK);
			return safetyhook::InlineHook{};
		}

		void* targetFunc = GetProcAddress(module, apiName);
		if (!targetFunc) 
		{
			char errorMsg[0x100];
			sprintf_s(errorMsg, "Failed to get API address: %s", apiName);
			MessageBoxA(NULL, errorMsg, "SafetyHook Error", MB_ICONERROR | MB_OK);
			return safetyhook::InlineHook{};
		}

		return CreateHook(targetFunc, hookFunc);
	}
}

namespace SystemHelper
{
	static std::string GetModulePath()
	{
		HMODULE hModule = nullptr;

		// Get the module handle for the DLL containing this code
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetModulePath, &hModule);

		wchar_t path[FILENAME_MAX] = { 0 };
		GetModuleFileNameW(hModule, path, FILENAME_MAX);
		return std::filesystem::path(path).parent_path().string();
	}

	static DWORD GetCurrentDisplayFrequency()
	{
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);

		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode))
		{
			return devMode.dmDisplayFrequency;
		}
		return 60;
	}

	static std::pair<DWORD, DWORD> GetScreenResolution()
	{
		DEVMODE devMode = {};
		devMode.dmSize = sizeof(DEVMODE);

		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode))
		{
			return { devMode.dmPelsWidth, devMode.dmPelsHeight };
		}
		return { 0, 0 };
	}

	static void LoadProxyLibrary()
	{
		// Attempt to load the chain-load DLL from the game's directory
		wchar_t modulePath[MAX_PATH];
		if (GetModuleFileNameW(NULL, modulePath, MAX_PATH))
		{
			wchar_t* lastBackslash = wcsrchr(modulePath, L'\\');
			if (lastBackslash != NULL)
			{
				*lastBackslash = L'\0';
				lstrcatW(modulePath, L"\\dinput8_hook.dll");

				HINSTANCE hChain = LoadLibraryExW(modulePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (hChain)
				{
					// Set up proxies to use the chain-loaded DLL
					if (dinput8.ProxySetup(hChain))
					{
						return; // Successfully chained
					}
					else
					{
						// Handle missing exports in chain DLL
						FreeLibrary(hChain);
						// Fall through to system DLL
					}
				}
			}
		}

		// Fallback to system dinput8.dll
		wchar_t systemPath[MAX_PATH];
		GetSystemDirectoryW(systemPath, MAX_PATH);
		lstrcatW(systemPath, L"\\dinput8.dll");

		HINSTANCE hOriginal = LoadLibraryExW(systemPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!hOriginal)
		{
			DWORD errorCode = GetLastError();
			wchar_t errorMessage[512];

			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), errorMessage, sizeof(errorMessage) / sizeof(wchar_t), NULL);
			MessageBoxW(NULL, errorMessage, L"Error Loading dinput8.dll", MB_ICONERROR);
			return;
		}

		// Set up proxies to system DLL
		dinput8.ProxySetup(hOriginal);
	}
};

namespace IniHelper
{
	mINI::INIFile iniFile(SystemHelper::GetModulePath() + "\\MarkerPatch.ini");
	mINI::INIStructure iniReader;

	void Init()
	{
		iniFile.read(iniReader);
	}

	void Save()
	{
		iniFile.write(iniReader);
	}

	char* ReadString(const char* sectionName, const char* valueName, const char* defaultValue)
	{
		char* result = new char[255];
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				std::string value = iniReader.get(sectionName).get(valueName);

				if (!value.empty() && (value.front() == '\"' || value.front() == '\''))
					value.erase(0, 1);
				if (!value.empty() && (value.back() == '\"' || value.back() == '\''))
					value.erase(value.size() - 1);

				strncpy(result, value.c_str(), 254);
				result[254] = '\0';
				return result;
			}
		}
		catch (...) {}

		strncpy(result, defaultValue, 254);
		result[254] = '\0';
		return result;
	}

	float ReadFloat(const char* sectionName, const char* valueName, float defaultValue)
	{
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				const std::string& s = iniReader.get(sectionName).get(valueName);
				if (!s.empty())
					return std::stof(s);
			}
		}
		catch (...) {}
		return defaultValue;
	}

	int ReadInteger(const char* sectionName, const char* valueName, int defaultValue)
	{
		try
		{
			if (iniReader.has(sectionName) && iniReader.get(sectionName).has(valueName))
			{
				const std::string& s = iniReader.get(sectionName).get(valueName);
				if (!s.empty())
					return std::stoi(s);
			}
		}
		catch (...) {}
		return defaultValue;
	}
};