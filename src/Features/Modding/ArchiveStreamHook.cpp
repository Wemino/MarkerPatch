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

	constexpr uint32_t BLOCK_SIZE = 0x20000;

	// UStreamer
	constexpr uintptr_t OFF_STREAMER_FREE_BUFFERS = 0x18;
	constexpr uintptr_t OFF_STREAMER_ACTIVE_STREAM = 0x48;
	constexpr uintptr_t OFF_STREAMER_LOAD_QUEUE = 0x4C;
	constexpr uintptr_t OFF_STREAMER_FREE_STREAMS = 0x5C;
	constexpr uintptr_t OFF_STREAMER_TOTAL_READ = 0x88;

	// UStream
	constexpr uintptr_t OFF_STREAM_NAME = 0x8;
	constexpr uintptr_t OFF_STREAM_FLAGS = 0xA8;
	constexpr uintptr_t OFF_STREAM_HANDLE = 0xAC;
	constexpr uintptr_t OFF_STREAM_STATUS = 0xB0;
	constexpr uintptr_t OFF_STREAM_FILE_SIZE = 0xBC;
	constexpr uintptr_t OFF_STREAM_FILE_READ = 0xC0;
	constexpr uintptr_t OFF_STREAM_FILE_RETIRED = 0xC4;
	constexpr uintptr_t OFF_STREAM_READING_BUFFER = 0xC8;
	constexpr uintptr_t OFF_STREAM_PENDING_BUFFERS = 0xD8;
	constexpr uintptr_t OFF_STREAM_PARSING_RESOURCE = 0x108;
	constexpr uintptr_t OFF_STREAM_RESOURCE_OFFSET = 0x10C;
	constexpr uintptr_t OFF_STREAM_PENDING_RESOURCES = 0x110;

	constexpr uint32_t STREAM_FLAG_CANCELLED = 2;
	constexpr int32_t STREAM_STATUS_OPENING = 1;
	constexpr int32_t STREAM_STATUS_IDLE = 2; // open, waiting for a free buffer
	constexpr int32_t STREAM_STATUS_READING = 3;

	// ThreadSafeQueue
	constexpr uintptr_t OFF_QUEUE_ARRAY = 0x0;
	constexpr uintptr_t OFF_QUEUE_CAPACITY = 0x4;
	constexpr uintptr_t OFF_QUEUE_START = 0x8;
	constexpr uintptr_t OFF_QUEUE_COUNT = 0xC;

	// UStreamBuffer
	constexpr uintptr_t OFF_BUFFER_DATA = 0x4;

	// TGameResourceRef
	constexpr uintptr_t OFF_REF_RESOURCE = 0x4;

	// TGameResource
	constexpr uintptr_t OFF_RES_STREAM_DATA = 0x08;
	constexpr uintptr_t OFF_RES_DATA_SIZE = 0x0C;

	// SHDR body, from the start of the SHOC chunk
	constexpr uintptr_t OFF_SHDR_GUID = 0x18;
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
		uint32_t handle = 0xFFFFFFFF;
		std::string relativePath;
		std::string key;
		bool skipPayload = false;
		StreamMap::Learner learner;
	};

	std::mutex g_lock;
	std::unordered_map<uintptr_t, StreamState> g_streams;

	// The file system's thread and the main thread
	std::recursive_mutex g_readLock;
	std::unordered_map<uintptr_t, StreamMap::Reader> g_readers;

	bool g_loadModFiles = false;
	bool g_skipArchiveData = false;
	bool g_learnStreamMaps = false;
	bool g_readAheadDone = false;

	safetyhook::InlineHook UStreamer_DispatchChunk;
	safetyhook::InlineHook UStreamer_Dispatch;
	safetyhook::InlineHook UStreamer_Load;
	safetyhook::InlineHook UStreamer_Init;
	safetyhook::InlineHook UStream_ReadCallback;
	safetyhook::InlineHook UStream_AttemptContinue;
	safetyhook::InlineHook UStream_Reset;
	safetyhook::InlineHook CResourceManager_Dispatch;
	safetyhook::InlineHook CResource_Destructor;

	static uintptr_t(__thiscall* UStreamer_DequeueFreeBuffer)(uintptr_t) = nullptr;
	static bool(__thiscall* UStreamBufferRing_Enqueue)(uintptr_t, uintptr_t) = nullptr;
	static void(__thiscall* UStream_AsyncClose)(uintptr_t) = nullptr;
	static void*(__cdecl* GameOperatorNew)(size_t) = nullptr;
	static uintptr_t g_streamerPtr = 0;

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

	static bool ReadHeader(const uint32_t* pChunk, AssetHeader& out)
	{
		const uint32_t chunkSize = pChunk[1];

		// Bounds the string walk below
		if (chunkSize < OFF_SHDR_STRINGS || chunkSize > BLOCK_SIZE) return false;

		const uint8_t* base = reinterpret_cast<const uint8_t*>(pChunk);
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

	static std::string GetStreamName(uintptr_t pStream)
	{
		const char* name = reinterpret_cast<const char*>(pStream + OFF_STREAM_NAME);
		return ModFiles::ToLower(std::string(name, strnlen(name, 0xA0)));
	}

	// The engine reuses its streams, a new handle means a new file
	static StreamState& GetStreamState(uintptr_t pStream)
	{
		const uint32_t handle = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_HANDLE);

		std::lock_guard<std::mutex> guard(g_lock);
		StreamState& state = g_streams[pStream];

		if (state.handle != handle)
		{
			state = StreamState();
			state.handle = handle;
		}

		return state;
	}

	// Under g_readLock
	static StreamMap::Reader& GetReader(uintptr_t pStream)
	{
		const uint32_t handle = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_HANDLE);
		StreamMap::Reader& reader = g_readers[pStream];

		if (reader.handle != handle)
		{
			reader = StreamMap::Reader();
			reader.handle = handle;
			reader.name = GetStreamName(pStream);
		}

		return reader;
	}

	static char HandleHeader(uintptr_t thisptr, uintptr_t pStream, uint32_t* pChunk, uintptr_t pBuffer, StreamState& state)
	{
		AssetHeader header{};
		const bool parsed = ReadHeader(pChunk, header);

		std::string relativePath = parsed ? BuildRelativePath(header) : "";
		std::string key = ModFiles::ToLower(relativePath);

		const ModFiles::Entry* entry = g_loadModFiles ? ModFiles::Find(key) : nullptr;

		// The engine sizes the resource from the header, so the loose file's size goes in first, it's known from the index
		uint32_t& sizeField = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pChunk) + OFF_SHDR_SIZE);
		const uint32_t archiveSize = sizeField;

		if (entry) sizeField = entry->size;

		const char result = UStreamer_DispatchChunk.unsafe_thiscall<char>(thisptr, pStream, pChunk, pBuffer);

		if (entry) sizeField = archiveSize;

		// Without a free dispatch slot, the engine gives the same chunk again later
		if (!result) return result;

		state.relativePath = std::move(relativePath);
		state.key = std::move(key);
		state.skipPayload = false;

		if (!entry) return result;

		const uintptr_t resource = *reinterpret_cast<uintptr_t*>(pStream + OFF_STREAM_PARSING_RESOURCE);

		// A null resource means the asset is already loaded and its payload is skipped
		if (resource == 0)
		{
			ModFiles::CancelReadAhead(state.key);
			return result;
		}

		void* data = *reinterpret_cast<void**>(resource + OFF_RES_STREAM_DATA);
		const uint32_t size = *reinterpret_cast<uint32_t*>(resource + OFF_RES_DATA_SIZE);

		// Guards against writing past the buffer
		if (data && size == entry->size)
		{
			ModFiles::AddLive(resource, reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(pChunk) + OFF_SHDR_GUID));
			ModFiles::Attach(state.key, entry, resource, data, size);
		}

		// The engine mounts the resource once its offset reaches its size, mounting doesn't touch the data
		*reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_RESOURCE_OFFSET) = size;

		uint32_t completion[3] = { FOURCC_SHOC, sizeof(completion), FOURCC_SDAT };

		UStreamer_DispatchChunk.unsafe_thiscall<char>(thisptr, pStream, completion, pBuffer);

		state.skipPayload = true;
		return result;
	}

	static char HandlePayload(uintptr_t thisptr, uintptr_t pStream, uint32_t* pChunk, uintptr_t pBuffer, StreamState& state)
	{
		if (state.skipPayload) return 1;

		const bool dump = DumpArchiveAssets && !state.relativePath.empty();
		const uintptr_t resource = dump ? *reinterpret_cast<uintptr_t*>(pStream + OFF_STREAM_PARSING_RESOURCE) : 0;

		const char result = UStreamer_DispatchChunk.unsafe_thiscall<char>(thisptr, pStream, pChunk, pBuffer);

		if (!dump || resource == 0) return result;

		// Cleared, so that fragment was the last one
		if (*reinterpret_cast<uintptr_t*>(pStream + OFF_STREAM_PARSING_RESOURCE) != 0) return result;

		const uintptr_t data = *reinterpret_cast<uintptr_t*>(resource + OFF_RES_STREAM_DATA);
		const uint32_t size = *reinterpret_cast<uint32_t*>(resource + OFF_RES_DATA_SIZE);

		if (data != 0)
		{
			ArchiveDump::Write(state.relativePath, reinterpret_cast<const void*>(data), size);
		}

		return result;
	}

	static void LearnChunk(uintptr_t pStream, StreamState& state, uint32_t position, const uint32_t* pChunk, bool isHeader)
	{
		if (position == 0)
		{
			StreamMap::Start(state.learner, *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_SIZE));
		}

		if (!StreamMap::Learn(state.learner, position, pChunk, isHeader ? &state.key : nullptr)) return;

		state.learner.active = false;
		StreamMap::Add(StreamMap::Finish(state.learner, GetStreamName(pStream)));
	}

	static char __fastcall UStreamer_DispatchChunk_Hook(uintptr_t thisptr, int, uintptr_t pStream, uint32_t* pChunk, uintptr_t pBuffer)
	{
		if (!pStream || !pChunk) return UStreamer_DispatchChunk.unsafe_thiscall<char>(thisptr, pStream, pChunk, pBuffer);

		StreamState& state = GetStreamState(pStream);
		const uint32_t position = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_RETIRED);
		const bool isShoc = pChunk[0] == FOURCC_SHOC && pChunk[1] >= 12;
		const bool isHeader = isShoc && pChunk[2] == FOURCC_SHDR;

		char result;

		if (isHeader)
		{
			result = HandleHeader(thisptr, pStream, pChunk, pBuffer, state);
		}
		else if (isShoc && (pChunk[2] == FOURCC_SDAT || pChunk[2] == FOURCC_RPAK))
		{
			result = HandlePayload(thisptr, pStream, pChunk, pBuffer, state);
		}
		else
		{
			result = UStreamer_DispatchChunk.unsafe_thiscall<char>(thisptr, pStream, pChunk, pBuffer);
		}

		if (g_learnStreamMaps && result)
		{
			LearnChunk(pStream, state, position, pChunk, isHeader);
		}

		return result;
	}

	// The headers in a block that just came off the disk, their loose files can start reading now
	static void ReadAheadHeaders(const uint8_t* data)
	{
		uint32_t offset = 0;

		while (offset + 12 <= BLOCK_SIZE)
		{
			const uint32_t* pChunk = reinterpret_cast<const uint32_t*>(data + offset);
			const uint32_t size = pChunk[1];

			if (size < 8 || size > BLOCK_SIZE - offset) break;

			AssetHeader header{};

			if (pChunk[0] == FOURCC_SHOC && pChunk[2] == FOURCC_SHDR && ReadHeader(pChunk, header))
			{
				const std::string key = ModFiles::ToLower(BuildRelativePath(header));
				const ModFiles::Entry* entry = ModFiles::Find(key);

				if (entry && !ModFiles::IsLive(reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(pChunk) + OFF_SHDR_GUID)))
				{
					ModFiles::ReadAhead(key, entry);
				}
			}

			offset += size;
		}
	}

	static void __fastcall UStream_ReadCallback_Hook(uintptr_t pStream)
	{
		const uint32_t flags = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FLAGS);
		const uintptr_t buffer = *reinterpret_cast<uintptr_t*>(pStream + OFF_STREAM_READING_BUFFER);
		const uint8_t* data = buffer ? *reinterpret_cast<const uint8_t**>(buffer + OFF_BUFFER_DATA) : nullptr;

		if (!(flags & STREAM_FLAG_CANCELLED) && data)
		{
			// Before the original, which starts the next read
			if (g_skipArchiveData)
			{
				const uint32_t block = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_READ) / BLOCK_SIZE;

				std::lock_guard<std::recursive_mutex> guard(g_readLock);
				StreamMap::Reader& reader = GetReader(pStream);

				if (block == 0)
				{
					StreamMap::Verify(reader, data, *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_SIZE));
				}
				else
				{
					StreamMap::Check(reader, block, data);
				}
			}

			if (ModFiles::g_readAheadEnabled)
			{
				ReadAheadHeaders(data);
			}
		}

		UStream_ReadCallback.unsafe_thiscall<void>(pStream);
	}

	static uint32_t GetFreeBufferCount()
	{
		const uintptr_t streamer = *reinterpret_cast<uintptr_t*>(g_streamerPtr);
		return streamer ? *reinterpret_cast<uint32_t*>(streamer + OFF_STREAMER_FREE_BUFFERS + OFF_QUEUE_COUNT) : 0;
	}

	// Done as the end of a read would, with one chunk the parser ignores in place of the data
	static bool SkipBlock(uintptr_t pStream, uint32_t position)
	{
		const uintptr_t streamer = *reinterpret_cast<uintptr_t*>(g_streamerPtr);
		const uintptr_t buffer = UStreamer_DequeueFreeBuffer(streamer);

		if (!buffer) return false;

		uint32_t* data = *reinterpret_cast<uint32_t**>(buffer + OFF_BUFFER_DATA);
		data[0] = StreamMap::FOURCC_SKIP;
		data[1] = BLOCK_SIZE;
		data[2] = 0;

		*reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_READ) = position + BLOCK_SIZE;
		*reinterpret_cast<uint32_t*>(streamer + OFF_STREAMER_TOTAL_READ) += BLOCK_SIZE;
		UStreamBufferRing_Enqueue(pStream + OFF_STREAM_PENDING_BUFFERS, buffer);
		return true;
	}

	// Starts the next read, called when a read ends or the file opened, and when a buffer is freed while the stream waits for one
	static void __fastcall UStream_AttemptContinue_Hook(uintptr_t pStream)
	{
		std::unique_lock<std::recursive_mutex> lock(g_readLock);

		for (;;)
		{
			const int32_t status = *reinterpret_cast<int32_t*>(pStream + OFF_STREAM_STATUS);

			if (*reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FLAGS) & STREAM_FLAG_CANCELLED)
			{
				lock.unlock();
				UStream_AttemptContinue.unsafe_thiscall<void>(pStream);
				return;
			}

			// A read running or the stream closing means another caller already continued it
			if (status != STREAM_STATUS_OPENING && status != STREAM_STATUS_IDLE && status != STREAM_STATUS_READING) return;
			if (status == STREAM_STATUS_READING && *reinterpret_cast<uintptr_t*>(pStream + OFF_STREAM_READING_BUFFER) != 0) return;

			const uint32_t position = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_READ);
			const uint32_t fileSize = *reinterpret_cast<uint32_t*>(pStream + OFF_STREAM_FILE_SIZE);

			if (position >= fileSize) return;

			if (!StreamMap::ShouldSkip(GetReader(pStream), position / BLOCK_SIZE))
			{
				UStream_AttemptContinue.unsafe_thiscall<void>(pStream);

				// A buffer freed while the stream was still reading would not have woken it
				if (*reinterpret_cast<int32_t*>(pStream + OFF_STREAM_STATUS) == STREAM_STATUS_IDLE)
				{
					MemoryBarrier();
					if (GetFreeBufferCount() > 0) continue;
				}

				return;
			}

			if (!SkipBlock(pStream, position))
			{
				// Same as the engine without a free buffer, then the same check as above
				InterlockedExchange(reinterpret_cast<volatile LONG*>(pStream + OFF_STREAM_STATUS), STREAM_STATUS_IDLE);

				if (GetFreeBufferCount() > 0) continue;
				return;
			}

			if (position + BLOCK_SIZE >= fileSize)
			{
				// Closing takes the streamer's lock, which the main thread can hold while it calls this
				lock.unlock();
				UStream_AsyncClose(pStream);
				return;
			}
		}
	}

	static uintptr_t FindStream(uintptr_t streamer, uint32_t handle)
	{
		for (uintptr_t stream = *reinterpret_cast<uintptr_t*>(streamer + OFF_STREAMER_LOAD_QUEUE); stream; stream = *reinterpret_cast<uintptr_t*>(stream))
		{
			if (*reinterpret_cast<uint32_t*>(stream + OFF_STREAM_HANDLE) == handle) return stream;
		}

		return 0;
	}

	static uint32_t __fastcall UStreamer_Dispatch_Hook(uintptr_t thisptr, int, uint32_t handle, char dataOnly, uint32_t timeAvailable)
	{
		// With a time limit, a resource whose loose file is still reading waits for the next frame instead of stalling this one
		if (!dataOnly && timeAvailable != 0xFFFFFFFF && ModFiles::HasPending())
		{
			const uintptr_t stream = FindStream(thisptr, handle);
			const uintptr_t first = stream ? *reinterpret_cast<uintptr_t*>(stream + OFF_STREAM_PENDING_RESOURCES) : 0;
			const uintptr_t resource = first ? *reinterpret_cast<uintptr_t*>(first + OFF_REF_RESOURCE) : 0;

			// Resources pending
			if (resource && !ModFiles::IsReady(resource)) return 4;
		}

		return UStreamer_Dispatch.unsafe_thiscall<uint32_t>(thisptr, handle, dataOnly, timeAvailable);
	}

	static void __fastcall CResourceManager_Dispatch_Hook(uintptr_t thisptr, int, uintptr_t resource)
	{
		ModFiles::Wait(resource);
		CResourceManager_Dispatch.unsafe_thiscall<void>(thisptr, resource);
	}

	static void __fastcall CResource_Destructor_Hook(uintptr_t thisptr)
	{
		// A read must not land in the data after it's freed
		ModFiles::Wait(thisptr);
		ModFiles::RemoveLive(thisptr);
		CResource_Destructor.unsafe_thiscall<void>(thisptr);
	}

	static int __fastcall UStream_Reset_Hook(uintptr_t pStream)
	{
		{
			std::lock_guard<std::mutex> guard(g_lock);
			g_streams.erase(pStream);
		}

		{
			std::lock_guard<std::recursive_mutex> guard(g_readLock);
			g_readers.erase(pStream);
		}

		return UStream_Reset.unsafe_thiscall<int>(pStream);
	}

	static void ExpandReadAhead()
	{
		const uintptr_t streamer = *reinterpret_cast<uintptr_t*>(g_streamerPtr);

		if (!streamer || g_readAheadDone) return;

		g_readAheadDone = true;

		const uintptr_t freeBuffers = streamer + OFF_STREAMER_FREE_BUFFERS;
		const uint32_t capacity = *reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_CAPACITY);
		const uint32_t count = *reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_COUNT);
		const uint32_t start = *reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_START);
		const uint32_t wanted = static_cast<uint32_t>(StreamReadAheadMB) * (1024 * 1024 / BLOCK_SIZE);

		// Only before anything streams, when every buffer is free
		if (wanted <= capacity || count != capacity) return;
		if (*reinterpret_cast<uintptr_t*>(streamer + OFF_STREAMER_LOAD_QUEUE) || *reinterpret_cast<uintptr_t*>(streamer + OFF_STREAMER_ACTIVE_STREAM)) return;

		std::vector<uintptr_t> streams;

		for (uintptr_t stream = *reinterpret_cast<uintptr_t*>(streamer + OFF_STREAMER_FREE_STREAMS); stream; stream = *reinterpret_cast<uintptr_t*>(stream))
		{
			if (*reinterpret_cast<uint32_t*>(stream + OFF_STREAM_PENDING_BUFFERS + OFF_QUEUE_COUNT) != 0) return;
			streams.push_back(stream);
		}

		const uint32_t extra = wanted - capacity;
		uint8_t* memory = static_cast<uint8_t*>(VirtualAlloc(NULL, static_cast<SIZE_T>(extra) * BLOCK_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		uint32_t* buffers = static_cast<uint32_t*>(VirtualAlloc(NULL, static_cast<SIZE_T>(extra) * 8, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		uintptr_t* freeArray = static_cast<uintptr_t*>(GameOperatorNew(wanted * sizeof(uintptr_t)));

		if (!memory || !buffers || !freeArray) return;

		std::vector<uintptr_t*> pendingArrays;

		for (size_t index = 0; index < streams.size(); index++)
		{
			uintptr_t* pendingArray = static_cast<uintptr_t*>(GameOperatorNew(wanted * sizeof(uintptr_t)));

			if (!pendingArray) return;

			pendingArrays.push_back(pendingArray);
		}

		const uintptr_t* oldArray = *reinterpret_cast<uintptr_t**>(freeBuffers + OFF_QUEUE_ARRAY);

		for (uint32_t index = 0; index < count; index++)
		{
			freeArray[index] = oldArray[(start + index) % capacity];
		}

		// UStreamBuffer: reference count, data
		for (uint32_t index = 0; index < extra; index++)
		{
			buffers[index * 2] = 0;
			buffers[index * 2 + 1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(memory + static_cast<size_t>(index) * BLOCK_SIZE));
			freeArray[count + index] = reinterpret_cast<uintptr_t>(&buffers[index * 2]);
		}

		*reinterpret_cast<uintptr_t**>(freeBuffers + OFF_QUEUE_ARRAY) = freeArray;
		*reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_CAPACITY) = wanted;
		*reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_START) = 0;
		*reinterpret_cast<uint32_t*>(freeBuffers + OFF_QUEUE_COUNT) = wanted;

		// Every stream must be able to hold every buffer
		for (size_t index = 0; index < streams.size(); index++)
		{
			const uintptr_t pendingBuffers = streams[index] + OFF_STREAM_PENDING_BUFFERS;
			*reinterpret_cast<uintptr_t**>(pendingBuffers + OFF_QUEUE_ARRAY) = pendingArrays[index];
			*reinterpret_cast<uint32_t*>(pendingBuffers + OFF_QUEUE_CAPACITY) = wanted;
			*reinterpret_cast<uint32_t*>(pendingBuffers + OFF_QUEUE_START) = 0;
		}
	}

	static uintptr_t __cdecl UStreamer_Init_Hook(uint32_t bufferBudget, uint32_t chunkCount)
	{
		const uintptr_t result = UStreamer_Init.unsafe_ccall<uintptr_t>(bufferBudget, chunkCount);
		ExpandReadAhead();
		return result;
	}

	// In case the streamer was set up before MarkerPatch, nothing streams before the first load either
	static uint32_t __fastcall UStreamer_Load_Hook(uintptr_t thisptr, int, const char* fileName, float priority, char isDLC)
	{
		ExpandReadAhead();
		return UStreamer_Load.unsafe_thiscall<uint32_t>(thisptr, fileName, priority, isDLC);
	}
}

static void ApplyArchiveStreamHook()
{
	using namespace ArchiveStream;

	g_streamerPtr = GetAddress(Addr::UStreamerPtr);
	UStreamer_DequeueFreeBuffer = reinterpret_cast<decltype(UStreamer_DequeueFreeBuffer)>(GetAddress(Addr::UStreamer_DequeueFreeBuffer));
	UStreamBufferRing_Enqueue = reinterpret_cast<decltype(UStreamBufferRing_Enqueue)>(GetAddress(Addr::UStreamBufferRing_Enqueue));
	UStream_AsyncClose = reinterpret_cast<decltype(UStream_AsyncClose)>(GetAddress(Addr::UStream_AsyncClose));
	GameOperatorNew = reinterpret_cast<decltype(GameOperatorNew)>(GetAddress(Addr::GameOperatorNew));

	g_loadModFiles = LoadModFiles && ModFiles::g_ready;

	if (g_loadModFiles && StreamReadAheadMB > 1)
	{
		UStreamer_Init = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStreamer_Init)), &UStreamer_Init_Hook);
		UStreamer_Load = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStreamer_Load)), &UStreamer_Load_Hook);
	}

	if (!DumpArchiveAssets && !g_loadModFiles) return;

	UStreamer_DispatchChunk = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStreamer_DispatchChunk)), &UStreamer_DispatchChunk_Hook);
	UStream_Reset = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStream_Reset)), &UStream_Reset_Hook);

	// Dumps learn the maps too
	if (SkipReplacedArchiveData && UStreamer_DispatchChunk)
	{
		StreamMap::Load(SystemHelper::GetModulePath() + "\\MarkerPatch_StreamMap.txt", ModFiles::GetModuleDirectoryW());
		g_learnStreamMaps = true;
	}

	if (!g_loadModFiles || !UStreamer_DispatchChunk) return;

	if (ModFiles::g_threadCount > 0)
	{
		CResourceManager_Dispatch = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::CResourceManager_Dispatch)), &CResourceManager_Dispatch_Hook);
		CResource_Destructor = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::CResource_Destructor)), &CResource_Destructor_Hook);

		// Reading in the background is only safe with both places that wait for a read, otherwise the main thread reads
		if (!CResourceManager_Dispatch || !CResource_Destructor)
		{
			ModFiles::g_threadCount = 0;
			ModFiles::g_readAheadEnabled = false;
		}
		else
		{
			UStreamer_Dispatch = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStreamer_Dispatch)), &UStreamer_Dispatch_Hook);
		}
	}

	g_skipArchiveData = SkipReplacedArchiveData;

	if (g_skipArchiveData || ModFiles::g_readAheadEnabled)
	{
		UStream_ReadCallback = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStream_ReadCallback)), &UStream_ReadCallback_Hook);
	}

	if (g_skipArchiveData)
	{
		UStream_AttemptContinue = HookHelper::CreateHook(reinterpret_cast<void*>(GetAddress(Addr::UStream_AttemptContinue)), &UStream_AttemptContinue_Hook);
	}
}
