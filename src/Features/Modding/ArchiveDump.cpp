#pragma once

#include "../../Globals.cpp"

// ================
// DumpArchiveAssets
// ================

namespace ArchiveDump
{
	std::string g_directory;
	std::unordered_set<std::string> g_written;
	std::mutex g_lock;
	bool g_ready = false;

	static void Write(const std::string& relativePath, const void* data, uint32_t size)
	{
		if (!g_ready || relativePath.empty() || size == 0) return;

		{
			std::lock_guard<std::mutex> guard(g_lock);

			if (!g_written.insert(relativePath).second) return;
		}

		const std::filesystem::path fullPath = BuildLongPath(g_directory, relativePath);

		std::error_code error;
		std::filesystem::create_directories(fullPath.parent_path(), error);

		if (error) return;

		// The archives never change, so anything already on disk is correct
		if (std::filesystem::exists(fullPath, error)) return;

		std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);

		if (!file) return;

		file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
	}
}

static void ApplyArchiveDump()
{
	if (!DumpArchiveAssets) return;

	ArchiveDump::g_directory = SystemHelper::GetModulePath() + "\\archive_dump";

	std::error_code error;
	std::filesystem::create_directories(ArchiveDump::g_directory, error);

	ArchiveDump::g_ready = !error;
}
