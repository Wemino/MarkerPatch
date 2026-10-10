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
		uint32_t size = 0;
		uint16_t mod = 0;
	};

	enum class State : uint8_t
	{
		Queued,
		Reading,
		Copying,
		Done
	};

	enum class Lane : uint8_t
	{
		None,
		Urgent, // The resource exists, the read goes straight into it
		ReadAhead // Only seen in the archive read-ahead, read into a buffer of ours
	};

	struct Request
	{
		std::string key;
		const Entry* entry = nullptr;
		uint32_t size = 0;

		State state = State::Queued;
		Lane lane = Lane::None;
		std::list<Request*>::iterator position;

		bool ok = false;
		bool orphan = false; // Nobody wants it anymore, freed once its read ends

		uint8_t* buffer = nullptr;
		void* target = nullptr;
		ULONGLONG created = 0;
	};

	struct Guid
	{
		uint32_t value[4];

		bool operator==(const Guid& other) const
		{
			return std::memcmp(value, other.value, sizeof(value)) == 0;
		}
	};

	struct GuidHash
	{
		size_t operator()(const Guid& guid) const
		{
			return guid.value[0] ^ (guid.value[1] * 31u) ^ (guid.value[2] * 131u) ^ (guid.value[3] * 1031u);
		}
	};

	std::vector<std::string> g_modNames;
	std::unordered_map<std::string, Entry> g_index;
	bool g_ready = false;

	std::mutex g_lock;
	std::condition_variable g_workSignal;
	std::condition_variable g_doneSignal;

	std::list<Request*> g_urgent;
	std::list<Request*> g_readAhead;
	std::unordered_map<std::string, Request*> g_readyAhead; // Read ahead, waiting for the header of their asset
	std::unordered_map<uintptr_t, Request*> g_pending; // Resource -> read that has to end before the resource is used
	std::atomic<int> g_pendingCount{ 0 };

	uint64_t g_bufferBytes = 0;
	uint64_t g_queuedAheadBytes = 0;
	uint64_t g_bufferBudget = 0;
	int g_threadCount = 0;
	bool g_readAheadEnabled = false;

	// Replaced resources the engine still holds
	std::mutex g_liveLock;
	std::unordered_map<uintptr_t, Guid> g_liveResources;
	std::unordered_map<Guid, uint32_t, GuidHash> g_liveGuids;
	std::atomic<int> g_liveCount{ 0 };

	static std::string ToLower(std::string value)
	{
		for (char& character : value)
		{
			character = static_cast<char>(::tolower(static_cast<unsigned char>(character)));
		}

		return value;
	}

	static const Entry* Find(const std::string& key)
	{
		if (!g_ready || key.empty()) return nullptr;

		const auto entry = g_index.find(key);
		return entry != g_index.end() ? &entry->second : nullptr;
	}

	static DWORD WINAPI ThreadEntry(LPVOID parameter)
	{
		reinterpret_cast<void (*)()>(parameter)();
		return 0;
	}

	// 256 KB of stack reserved instead of 1 MB, address space is short in a 32-bit game
	static bool StartThread(void (*function)())
	{
		const HANDLE thread = CreateThread(NULL, 256 * 1024, ThreadEntry, reinterpret_cast<LPVOID>(function), STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

		if (!thread) return false;

		CloseHandle(thread);
		return true;
	}

	// The sizes come along with the names, FindExInfoBasic and FIND_FIRST_EX_LARGE_FETCH need Windows 7
	static HANDLE FindFirst(const std::wstring& pattern, WIN32_FIND_DATAW& data)
	{
		HANDLE find = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

		if (find == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
		{
			find = FindFirstFileExW(pattern.c_str(), FindExInfoStandard, &data, FindExSearchNameMatch, NULL, 0);
		}

		return find;
	}

	// What is missing is zeroed when the file became shorter since the index was built
	static bool ReadInto(const Entry& entry, void* destination, uint32_t size)
	{
		uint8_t* cursor = static_cast<uint8_t*>(destination);
		uint32_t left = size;

		const HANDLE handle = CreateFileW(entry.fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (handle != INVALID_HANDLE_VALUE)
		{
			while (left > 0)
			{
				DWORD read = 0;

				if (!ReadFile(handle, cursor, left, &read, NULL) || read == 0) break;

				cursor += read;
				left -= read;
			}

			CloseHandle(handle);
		}

		if (left == 0) return true;

		std::memset(cursor, 0, left);
		return false;
	}

	static void Unqueue(Request* request)
	{
		if (request->lane == Lane::Urgent)
		{
			g_urgent.erase(request->position);
		}
		else if (request->lane == Lane::ReadAhead)
		{
			g_readAhead.erase(request->position);
			g_queuedAheadBytes -= request->size;
		}

		request->lane = Lane::None;
	}

	static void QueueUrgent(Request* request)
	{
		Unqueue(request);
		g_urgent.push_back(request);
		request->position = std::prev(g_urgent.end());
		request->lane = Lane::Urgent;
	}

	static void FreeBuffer(Request* request)
	{
		if (!request->buffer) return;

		std::free(request->buffer);
		request->buffer = nullptr;
		g_bufferBytes -= request->size;
	}

	// Under g_lock: queued requests are deleted, reading ones are freed by their thread
	static void Drop(Request* request)
	{
		if (request->state == State::Queued)
		{
			Unqueue(request);
			delete request;
		}
		else if (request->state == State::Done)
		{
			FreeBuffer(request);
			delete request;
		}
		else
		{
			request->orphan = true;
		}
	}

	static Request* PickLocked()
	{
		if (!g_urgent.empty())
		{
			Request* request = g_urgent.front();
			Unqueue(request);
			return request;
		}

		if (!g_readAhead.empty())
		{
			Request* request = g_readAhead.front();

			// In order, a big file waits for room rather than being passed by the small ones
			if (g_bufferBytes == 0 || g_bufferBytes + request->size <= g_bufferBudget)
			{
				Unqueue(request);
				return request;
			}
		}

		return nullptr;
	}

	static void Worker()
	{
		std::unique_lock<std::mutex> lock(g_lock);

		for (;;)
		{
			Request* request = nullptr;
			g_workSignal.wait(lock, [&] { return (request = PickLocked()) != nullptr; });

			request->state = State::Reading;

			void* destination = request->target;

			if (!destination)
			{
				request->buffer = static_cast<uint8_t*>(std::malloc(request->size));
				destination = request->buffer;

				if (request->buffer) g_bufferBytes += request->size;
			}

			lock.unlock();

			const bool ok = destination && ReadInto(*request->entry, destination, request->size);

			lock.lock();

			request->ok = ok;

			if (request->orphan)
			{
				FreeBuffer(request);
				delete request;
				continue;
			}

			// The resource showed up while we were reading into our buffer, or there was no buffer
			if (request->target && destination != request->target)
			{
				request->state = State::Copying;
				const uint8_t* buffer = request->buffer;
				lock.unlock();

				if (ok && buffer)
				{
					std::memcpy(request->target, buffer, request->size);
				}
				else
				{
					request->ok = ReadInto(*request->entry, request->target, request->size);
				}

				lock.lock();
				FreeBuffer(request);
			}

			// A failed read ahead is forgotten, the header starts a new one
			if (!request->target && !ok)
			{
				g_readyAhead.erase(request->key);
				FreeBuffer(request);
				delete request;
				continue;
			}

			request->state = State::Done;
			g_doneSignal.notify_all();
		}
	}

	// What a cancelled stream read ahead would otherwise be kept forever
	static void SweepLocked(ULONGLONG now)
	{
		for (auto entry = g_readyAhead.begin(); entry != g_readyAhead.end();)
		{
			if (now - entry->second->created < 30000)
			{
				++entry;
				continue;
			}

			Drop(entry->second);
			entry = g_readyAhead.erase(entry);
		}
	}

	// I/O thread: the header of this asset just came off the disk
	static void ReadAhead(const std::string& key, const Entry* entry)
	{
		if (!g_readAheadEnabled || !entry) return;

		std::lock_guard<std::mutex> guard(g_lock);

		if (g_readyAhead.count(key)) return;

		// Bounds what piles up when the main thread falls behind
		if (g_queuedAheadBytes > 0 && g_queuedAheadBytes + g_bufferBytes + entry->size > g_bufferBudget * 2) return;

		const ULONGLONG now = GetTickCount64();
		SweepLocked(now);

		Request* request = new Request();
		request->key = key;
		request->entry = entry;
		request->size = entry->size;
		request->created = now;

		g_readAhead.push_back(request);
		request->position = std::prev(g_readAhead.end());
		request->lane = Lane::ReadAhead;
		g_queuedAheadBytes += request->size;

		g_readyAhead.emplace(key, request);
		g_workSignal.notify_one();
	}

	// The engine already had the asset, nothing will claim what was read ahead
	static void CancelReadAhead(const std::string& key)
	{
		if (!g_readAheadEnabled) return;

		std::lock_guard<std::mutex> guard(g_lock);

		const auto entry = g_readyAhead.find(key);

		if (entry == g_readyAhead.end()) return;

		Drop(entry->second);
		g_readyAhead.erase(entry);
	}

	// Main thread: the engine created the resource of a replaced asset, its data goes into "target"
	static void Attach(const std::string& key, const Entry* entry, uintptr_t resource, void* target, uint32_t size)
	{
		if (g_threadCount == 0)
		{
			ReadInto(*entry, target, size);
			return;
		}

		std::unique_lock<std::mutex> lock(g_lock);

		Request* request = nullptr;
		const auto ahead = g_readyAhead.find(key);

		if (ahead != g_readyAhead.end())
		{
			request = ahead->second;
			g_readyAhead.erase(ahead);

			if (request->size != size)
			{
				Drop(request);
				request = nullptr;
			}
		}

		// Read ahead in time, only a copy left
		if (request && request->state == State::Done)
		{
			uint8_t* buffer = request->buffer;
			request->buffer = nullptr;
			g_bufferBytes -= size;
			lock.unlock();

			std::memcpy(target, buffer, size);
			std::free(buffer);
			delete request;
			return;
		}

		if (request)
		{
			request->target = target;

			// Not started yet: it goes straight into the resource, ahead of the read-ahead
			if (request->state == State::Queued)
			{
				QueueUrgent(request);
				g_workSignal.notify_one();
			}
		}
		else
		{
			request = new Request();
			request->key = key;
			request->entry = entry;
			request->size = size;
			request->target = target;

			QueueUrgent(request);
			g_workSignal.notify_one();
		}

		g_pending[resource] = request;
		g_pendingCount.fetch_add(1, std::memory_order_release);
	}

	static bool HasPending()
	{
		return g_pendingCount.load(std::memory_order_acquire) > 0;
	}

	static bool IsReady(uintptr_t resource)
	{
		if (!HasPending()) return true;

		std::lock_guard<std::mutex> guard(g_lock);

		const auto entry = g_pending.find(resource);
		return entry == g_pending.end() || entry->second->state == State::Done;
	}

	// Before the engine uses or frees the data of a resource
	static void Wait(uintptr_t resource)
	{
		if (!HasPending()) return;

		std::unique_lock<std::mutex> lock(g_lock);

		const auto entry = g_pending.find(resource);

		if (entry == g_pending.end()) return;

		Request* request = entry->second;

		// Still queued behind other files, quicker to read it here than to wait for a thread
		if (request->state == State::Queued)
		{
			Unqueue(request);
			request->state = State::Reading;
			lock.unlock();

			const bool ok = ReadInto(*request->entry, request->target, request->size);

			lock.lock();
			request->ok = ok;
			request->state = State::Done;
		}

		g_doneSignal.wait(lock, [&] { return request->state == State::Done; });

		g_pending.erase(resource);
		g_pendingCount.fetch_sub(1, std::memory_order_release);
		delete request;
	}

	static bool IsNullGuid(const uint32_t* value)
	{
		return (value[0] | value[1] | value[2] | value[3]) == 0;
	}

	static void AddLive(uintptr_t resource, const uint32_t* value)
	{
		if (!g_readAheadEnabled || IsNullGuid(value)) return;

		Guid guid;
		std::memcpy(guid.value, value, sizeof(guid.value));

		std::lock_guard<std::mutex> guard(g_liveLock);

		if (g_liveResources.emplace(resource, guid).second)
		{
			g_liveGuids[guid]++;
			g_liveCount.fetch_add(1, std::memory_order_relaxed);
		}
	}

	static void RemoveLive(uintptr_t resource)
	{
		if (g_liveCount.load(std::memory_order_relaxed) == 0) return;

		std::lock_guard<std::mutex> guard(g_liveLock);

		const auto entry = g_liveResources.find(resource);

		if (entry == g_liveResources.end()) return;

		const auto guid = g_liveGuids.find(entry->second);

		if (guid != g_liveGuids.end() && --guid->second == 0) g_liveGuids.erase(guid);

		g_liveResources.erase(entry);
		g_liveCount.fetch_sub(1, std::memory_order_relaxed);
	}

	// The engine skips the payload of an asset it holds, reading it ahead would be for nothing
	static bool IsLive(const uint32_t* value)
	{
		if (IsNullGuid(value) || g_liveCount.load(std::memory_order_relaxed) == 0) return false;

		Guid guid;
		std::memcpy(guid.value, value, sizeof(guid.value));

		std::lock_guard<std::mutex> guard(g_liveLock);
		return g_liveGuids.count(guid) != 0;
	}

	static void StartThreads(int threads, uint64_t budget)
	{
		g_bufferBudget = budget;

		for (int index = 0; index < threads; index++)
		{
			if (StartThread(Worker)) g_threadCount++;
		}

		// Without threads, the main thread reads the files itself
		g_readAheadEnabled = g_threadCount > 0 && g_bufferBudget > 0;
	}

	static std::string Narrow(const wchar_t* text, size_t length)
	{
		if (length == 0) return "";

		const int size = WideCharToMultiByte(CP_ACP, 0, text, static_cast<int>(length), NULL, 0, NULL, NULL);
		std::string result(size, '\0');
		WideCharToMultiByte(CP_ACP, 0, text, static_cast<int>(length), result.data(), size, NULL, NULL);
		return result;
	}

	static std::wstring GetModuleDirectoryW()
	{
		HMODULE module = nullptr;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&GetModuleDirectoryW), &module);

		std::wstring path(32768, L'\0');
		path.resize(GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size())));

		const size_t separator = path.find_last_of(L"\\/");
		if (separator != std::wstring::npos) path.resize(separator);

		return path;
	}

	static void IndexMod(const std::wstring& root, uint16_t mod, std::unordered_map<std::string, std::pair<std::string, int>>& ignoredMods)
	{
		std::vector<std::wstring> folders{ root };

		while (!folders.empty())
		{
			const std::wstring folder = std::move(folders.back());
			folders.pop_back();

			WIN32_FIND_DATAW data;
			const HANDLE find = FindFirst(folder + L"\\*", data);

			if (find == INVALID_HANDLE_VALUE) continue;

			do
			{
				const wchar_t* name = data.cFileName;

				if (name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0'))) continue;

				std::wstring fullPath = folder + L"\\" + name;

				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Links could loop
					if (!(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) folders.push_back(std::move(fullPath));
					continue;
				}

				const uint64_t size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;

				// The engine has no use for an empty asset, and anything past 4GB would not survive the cast
				if (size == 0 || size > 0xFFFFFFFF) continue;

				std::string relativePath = Narrow(fullPath.c_str() + root.size() + 1, fullPath.size() - root.size() - 1);

				for (char& character : relativePath)
				{
					if (character == '/') character = '\\';
				}

				Entry record;
				record.fullPath = std::move(fullPath);
				record.size = static_cast<uint32_t>(size);
				record.mod = mod;

				const auto inserted = g_index.emplace(ToLower(relativePath), std::move(record));

				if (!inserted.second)
				{
					std::pair<std::string, int>& ignored = ignoredMods[g_modNames[mod]];

					if (ignored.second == 0)
					{
						ignored.first = g_modNames[inserted.first->second.mod];
					}

					ignored.second++;
				}
			}
			while (FindNextFileW(find, &data));

			FindClose(find);
		}
	}
}

static void ApplyModFiles()
{
	if (!LoadModFiles) return;

	// \\?\ lifts the 260 character path limit
	std::wstring root = ModFiles::GetModuleDirectoryW() + L"\\mods";

	if (root.rfind(L"\\\\?\\", 0) != 0)
	{
		root = root.rfind(L"\\\\", 0) == 0 ? L"\\\\?\\UNC\\" + root.substr(2) : L"\\\\?\\" + root;
	}

	std::vector<std::wstring> modFolders;

	WIN32_FIND_DATAW data;
	const HANDLE find = ModFiles::FindFirst(root + L"\\*", data);

	if (find == INVALID_HANDLE_VALUE) return;

	do
	{
		const wchar_t* name = data.cFileName;

		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
		if (name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0'))) continue;

		modFolders.emplace_back(name);
	}
	while (FindNextFileW(find, &data));

	FindClose(find);

	// Name order, so the winner of a conflict never changes between runs
	std::sort(modFolders.begin(), modFolders.end());

	std::unordered_map<std::string, std::pair<std::string, int>> ignoredMods; // mod -> (mod used instead, file count)

	for (const std::wstring& folder : modFolders)
	{
		const uint16_t mod = static_cast<uint16_t>(ModFiles::g_modNames.size());
		ModFiles::g_modNames.push_back(ModFiles::Narrow(folder.c_str(), folder.size()));
		ModFiles::IndexMod(root + L"\\" + folder, mod, ignoredMods);
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

	if (!ModFiles::g_ready) return;

	ModFiles::StartThreads(LooseFileThreads, static_cast<uint64_t>(LooseFilePrefetchMB) * 1024 * 1024);
}
