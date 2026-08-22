#pragma once

#include "../../Globals.cpp"

// =================
// ArchiveStreamHook
// =================
//
// Shared hook for DumpArchiveAssets and LoadModFiles

namespace ArchiveStream
{
	// Tags
	constexpr uint32_t FOURCC_SHOC = 0x53484F43;
	constexpr uint32_t FOURCC_SHDR = 0x53484452;
	constexpr uint32_t FOURCC_SDAT = 0x53444154;
	constexpr uint32_t FOURCC_RPAK = 0x5270616B;

	// UStream
	constexpr uintptr_t OFF_STREAM_RESOURCE = 0x108;
	constexpr uintptr_t OFF_STREAM_OFFSET = 0x10C;

	// Resource
	constexpr uintptr_t OFF_RES_DATA = 0x08;
	constexpr uintptr_t OFF_RES_SIZE = 0x0C;

	// SHDR body, from the start of the SHOC chunk
	constexpr uintptr_t OFF_SHDR_SIZE = 0x28;
	constexpr uintptr_t OFF_SHDR_STRINGS = 0x2C;

	struct AssetHeader
	{
		uint32_t size;
		char name[128];
		char path[260];
		char type[32];
	};

	struct StreamState
	{
		std::string relativePath;
		bool skipPayload = false;
	};

	std::mutex g_lock;
	std::unordered_map<uintptr_t, StreamState> g_streams;

	safetyhook::InlineHook DispatchChunk;

	static void CopyBounded(char* destination, size_t destinationSize, const char*& source, const char* end)
	{
		size_t index = 0;

		while (source < end && *source && index + 1 < destinationSize)
		{
			destination[index++] = *source++;
		}

		destination[index] = '\0';

		while (source < end && *source) ++source; // skip the rest if it did not fit
		if (source < end) ++source;
	}

	static bool ReadHeader(const uint32_t* chunk, AssetHeader& out)
	{
		const uint32_t chunkSize = chunk[1];

		// Bounds the string walk below
		if (chunkSize < OFF_SHDR_STRINGS || chunkSize > 0x20000) return false;

		const uint8_t* base = reinterpret_cast<const uint8_t*>(chunk);
		out.size = *reinterpret_cast<const uint32_t*>(base + OFF_SHDR_SIZE);

		const char* cursor = reinterpret_cast<const char*>(base + OFF_SHDR_STRINGS);
		const char* end = reinterpret_cast<const char*>(base + chunkSize);
		CopyBounded(out.name, sizeof(out.name), cursor, end);
		CopyBounded(out.path, sizeof(out.path), cursor, end);
		CopyBounded(out.type, sizeof(out.type), cursor, end);
		return true;
	}

	// One path can hold two asset types, so the tag is appended unless the extension already says it
	static std::string BuildRelativePath(const AssetHeader& header)
	{
		std::string source = header.path[0] ? header.path : header.name;

		if (source.empty()) return "";

		std::string result;
		std::string part;

		auto flush = [&]()
		{
			if (!part.empty() && part != "." && part != "..")
			{
				if (!result.empty()) result += '\\';
				result += part;
			}

			part.clear();
		};

		for (char character : source)
		{
			if (character == '\\' || character == '/')
			{
				flush();
				continue;
			}

			if (character == ':' || character == '*' || character == '?' || character == '"' || character == '<' || character == '>' || character == '|')
			{
				character = '_';
			}

			if (static_cast<unsigned char>(character) < 0x20) character = '_';

			part += character;
		}

		flush();

		if (result.empty()) return "";

		const size_t dot = result.find_last_of('.');
		const size_t separator = result.find_last_of('\\');
		const bool hasExtension = dot != std::string::npos && (separator == std::string::npos || dot > separator);

		if (header.type[0] && (!hasExtension || _stricmp(result.c_str() + dot + 1, header.type) != 0))
		{
			result += '.';
			result += header.type;
		}

		return result;
	}

	static char HandleHeader(uintptr_t thisptr, uintptr_t stream, uint32_t* chunk, uintptr_t buffer)
	{
		AssetHeader header{};
		StreamState state;

		const bool parsed = ReadHeader(chunk, header);

		if (parsed)
		{
			state.relativePath = BuildRelativePath(header);
		}

		// The engine sizes the resource from the header, so the loose file's size goes in first
		ModFiles::File modFile;
		bool patched = false;

		if (parsed && LoadModFiles && ModFiles::Open(state.relativePath, modFile))
		{
			patched = MemoryHelper::WriteMemory<uint32_t>(reinterpret_cast<uintptr_t>(chunk) + OFF_SHDR_SIZE, modFile.size);
		}

		const char result = DispatchChunk.unsafe_thiscall<char>(reinterpret_cast<void*>(thisptr), stream, chunk, buffer);

		const uintptr_t resource = *reinterpret_cast<uintptr_t*>(stream + OFF_STREAM_RESOURCE);

		if (patched)
		{
			MemoryHelper::WriteMemory<uint32_t>(reinterpret_cast<uintptr_t>(chunk) + OFF_SHDR_SIZE, header.size);

			// A null resource means the asset is already loaded and its payload is skipped
			if (resource != 0)
			{
				const uintptr_t data = *reinterpret_cast<uintptr_t*>(resource + OFF_RES_DATA);
				const uint32_t size = *reinterpret_cast<uint32_t*>(resource + OFF_RES_SIZE);

				// Guards against reading past the buffer
				if (data != 0 && size == modFile.size)
				{
					ModFiles::Read(modFile, reinterpret_cast<void*>(data));
				}

				// The engine mounts the resource once its offset reaches its size
				MemoryHelper::WriteMemory<uint32_t>(stream + OFF_STREAM_OFFSET, size);

				uint32_t completion[3] = { FOURCC_SHOC, sizeof(completion), FOURCC_SDAT };

				DispatchChunk.unsafe_thiscall<char>(reinterpret_cast<void*>(thisptr), stream, completion, buffer);

				state.skipPayload = true;
			}
		}

		std::lock_guard<std::mutex> guard(g_lock);
		g_streams[stream] = std::move(state);

		return result;
	}

	static char HandlePayload(uintptr_t thisptr, uintptr_t stream, uint32_t* chunk, uintptr_t buffer)
	{
		std::string relativePath;

		{
			std::lock_guard<std::mutex> guard(g_lock);

			const auto entry = g_streams.find(stream);

			if (entry != g_streams.end())
			{
				if (entry->second.skipPayload) return 1;

				if (DumpArchiveAssets)
				{
					relativePath = entry->second.relativePath;
				}
			}
		}

		const uintptr_t resource = *reinterpret_cast<uintptr_t*>(stream + OFF_STREAM_RESOURCE);

		const char result = DispatchChunk.unsafe_thiscall<char>(reinterpret_cast<void*>(thisptr), stream, chunk, buffer);

		if (!DumpArchiveAssets || resource == 0 || relativePath.empty()) return result;

		// Cleared, so that fragment was the last one
		if (*reinterpret_cast<uintptr_t*>(stream + OFF_STREAM_RESOURCE) != 0) return result;

		const uintptr_t data = *reinterpret_cast<uintptr_t*>(resource + OFF_RES_DATA);
		const uint32_t size = *reinterpret_cast<uint32_t*>(resource + OFF_RES_SIZE);

		if (data != 0)
		{
			ArchiveDump::Write(relativePath, reinterpret_cast<const void*>(data), size);
		}

		return result;
	}

	static char __fastcall DispatchChunk_Hook(uintptr_t thisptr, int, uintptr_t stream, uint32_t* chunk, uintptr_t buffer)
	{
		if (stream && chunk && chunk[0] == FOURCC_SHOC)
		{
			const uint32_t kind = chunk[2];

			if (kind == FOURCC_SHDR) return HandleHeader(thisptr, stream, chunk, buffer);
			if (kind == FOURCC_SDAT || kind == FOURCC_RPAK) return HandlePayload(thisptr, stream, chunk, buffer);
		}

		return DispatchChunk.unsafe_thiscall<char>(reinterpret_cast<void*>(thisptr), stream, chunk, buffer);
	}
}

static void ApplyArchiveStreamHook()
{
	if (!DumpArchiveAssets && !LoadModFiles) return;

	DWORD addr_DispatchChunk = ScanModuleSignature(g_State.GameModule, "51 53 57 8B 7C 24 14 8B 07 8B D9 C6 44 24 0B 01 ", "DispatchChunk");

	if (addr_DispatchChunk == 0) return;

	ArchiveStream::DispatchChunk = HookHelper::CreateHook(reinterpret_cast<void*>(addr_DispatchChunk), &ArchiveStream::DispatchChunk_Hook);
}
