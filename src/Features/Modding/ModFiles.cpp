#pragma once

#include "../../Globals.cpp"

// ============
// LoadModFiles
// ============
//
// Loose files from mods\<mod name>\<path the asset has inside the archive>

namespace ModFiles
{
	struct Entry
	{
		std::wstring fullPath;
		std::string mod;
	};

	// Kept open between the size and the read
	struct File
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
		uint32_t size = 0;

		~File()
		{
			if (handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);
			}
		}
	};

	std::string g_directory;
	std::unordered_map<std::string, Entry> g_index;
	bool g_ready = false;

	static std::string ToLower(std::string value)
	{
		for (char& character : value)
		{
			character = static_cast<char>(::tolower(static_cast<unsigned char>(character)));
		}

		return value;
	}

	static bool Open(const std::string& relativePath, File& file)
	{
		if (!g_ready || relativePath.empty()) return false;

		const auto entry = g_index.find(ToLower(relativePath));

		if (entry == g_index.end()) return false;

		file.handle = CreateFileW(entry->second.fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (file.handle == INVALID_HANDLE_VALUE) return false;

		LARGE_INTEGER size;

		// Anything past 4GB would not survive the cast
		if (!GetFileSizeEx(file.handle, &size) || size.QuadPart <= 0 || size.QuadPart > 0xFFFFFFFF) return false;

		file.size = static_cast<uint32_t>(size.QuadPart);
		return true;
	}

	// Reads into the engine's resource buffer, which the caller has already sized
	static bool Read(const File& file, void* destination)
	{
		DWORD read = 0;
		return ReadFile(file.handle, destination, file.size, &read, NULL) && read == file.size;
	}
}

static void ApplyModFiles()
{
	if (!LoadModFiles) return;

	ModFiles::g_directory = SystemHelper::GetModulePath() + "\\mods";

	std::error_code error;

	if (!std::filesystem::is_directory(ModFiles::g_directory, error)) return;

	std::vector<std::filesystem::path> modFolders;

	for (const auto& entry : std::filesystem::directory_iterator(ModFiles::g_directory, error))
	{
		if (entry.is_directory(error)) modFolders.push_back(entry.path());
	}

	// Name order, so the winner of a conflict never changes between runs
	std::sort(modFolders.begin(), modFolders.end());

	std::unordered_map<std::string, std::pair<std::string, int>> ignoredMods; // mod -> (mod used instead, file count)

	for (const std::filesystem::path& folder : modFolders)
	{
		const std::string modName = folder.filename().string();

		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied, error))
			{
				if (!entry.is_regular_file(error)) continue;

				std::string relativePath = std::filesystem::relative(entry.path(), folder, error).string();

				if (error || relativePath.empty()) continue;

				for (char& character : relativePath)
				{
					if (character == '/') character = '\\';
				}

				ModFiles::Entry record;

				// \\?\ lifts the 260 character path limit
				record.fullPath = L"\\\\?\\" + entry.path().wstring();
				record.mod = modName;

				const auto inserted = ModFiles::g_index.emplace(ModFiles::ToLower(relativePath), std::move(record));

				if (!inserted.second)
				{
					std::pair<std::string, int>& ignored = ignoredMods[modName];

					if (ignored.second == 0)
					{
						ignored.first = inserted.first->second.mod;
					}

					ignored.second++;
				}
			}
		}
		catch (...) {}
	}

	if (!ignoredMods.empty())
	{
		std::string message = "More than one mod provides the same files.\n" "The first mod in alphabetical order is used:\n";

		for (const auto& entry : ignoredMods)
		{
			message += "\n    \"" + entry.first + "\": " + std::to_string(entry.second.second) + " file(s) ignored, \"" + entry.second.first + "\" used instead";
		}

		MessageBoxA(NULL, message.c_str(), "MarkerPatch", MB_ICONWARNING);
	}

	ModFiles::g_ready = !ModFiles::g_index.empty();
}
