#pragma once

#include "../../Globals.cpp"

// =========
// StreamMap
// =========
//
// MarkerPatch_StreamMap.txt, where each asset sits in each stream

namespace StreamMap
{
	constexpr uint32_t BLOCK_SIZE = 0x20000;
	constexpr uint32_t SAMPLE_SIZE = 0x1000;

	// Tags
	constexpr uint32_t FOURCC_SHOC = 0x53484F43;
	constexpr uint32_t FOURCC_SHDR = 0x53484452;
	constexpr uint32_t FOURCC_SDAT = 0x53444154;
	constexpr uint32_t FOURCC_RPAK = 0x5270616B;
	constexpr uint32_t FOURCC_SKIP = 0x4B53504D; // stands for a block that was not read

	struct Range
	{
		uint32_t first = 0;
		uint32_t count = 0;
		std::string key;
	};

	struct Map
	{
		std::string name;
		uint32_t fileSize = 0;
		uint64_t hash = 0; // first block
		std::vector<uint32_t> samples; // every block, from its first and last 4 KB
		std::vector<Range> ranges; // blocks holding only the payload of one asset
		mutable std::atomic<bool> invalid{ false };
	};

	// Main thread, fed every chunk the engine parses, in order
	struct Learner
	{
		bool active = false;
		uint32_t fileSize = 0;
		uint64_t next = 0;
		uint64_t hash = 0;
		std::vector<uint32_t> samples;
		std::vector<int32_t> owners; // -1: nothing yet, -2: has to be read, otherwise the key
		std::vector<std::string> keys;
		std::unordered_map<std::string, int32_t> ids;
		int32_t current = -2;
	};

	// Per stream being read, under the hook's lock
	struct Reader
	{
		uint32_t handle = 0xFFFFFFFF;
		std::string name;
		const Map* map = nullptr;
		std::vector<uint8_t> skip;
	};

	std::mutex g_lock;
	std::unordered_map<std::string, std::vector<std::unique_ptr<Map>>> g_maps;
	std::string g_path;
	std::string g_fingerprint;
	bool g_fileValid = false;

	static uint64_t Hash64(const uint8_t* data, size_t size, uint64_t hash = 0xCBF29CE484222325ull)
	{
		size_t index = 0;

		for (; index + 8 <= size; index += 8)
		{
			uint64_t word;
			std::memcpy(&word, data + index, 8);
			hash = (hash ^ word) * 0x100000001B3ull;
			hash ^= hash >> 29;
		}

		for (; index < size; index++)
		{
			hash = (hash ^ data[index]) * 0x100000001B3ull;
		}

		return hash;
	}

	static uint64_t HashBlock(const uint8_t* block)
	{
		return Hash64(block, BLOCK_SIZE);
	}

	static uint32_t SampleBlock(const uint8_t* block)
	{
		const uint64_t hash = Hash64(block + BLOCK_SIZE - SAMPLE_SIZE, SAMPLE_SIZE, Hash64(block, SAMPLE_SIZE));
		return static_cast<uint32_t>(hash ^ (hash >> 32));
	}

	// Size and date of the game's archives
	static std::string Fingerprint(const std::wstring& folder)
	{
		std::vector<std::string> lines;

		WIN32_FIND_DATAW data;
		const HANDLE find = ModFiles::FindFirst(folder + L"\\DS2DAT*.DAT", data);

		if (find != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

				const uint64_t size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
				const uint64_t time = (static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime;
				const std::string name = ModFiles::ToLower(ModFiles::Narrow(data.cFileName, wcslen(data.cFileName)));

				char line[512];
				snprintf(line, sizeof(line), "D %llu %llu %s\n", static_cast<unsigned long long>(size), static_cast<unsigned long long>(time), name.c_str());
				lines.emplace_back(line);
			}
			while (FindNextFileW(find, &data));

			FindClose(find);
		}

		std::sort(lines.begin(), lines.end());

		std::string result;

		for (const std::string& line : lines)
		{
			result += line;
		}

		return result;
	}

	static void CommitLocked(std::unique_ptr<Map> map)
	{
		std::vector<std::unique_ptr<Map>>& list = g_maps[map->name];

		// A newer copy of the same stream replaces the old one
		for (auto& existing : list)
		{
			if (existing->fileSize == map->fileSize && existing->hash == map->hash)
			{
				existing = std::move(map);
				return;
			}
		}

		list.push_back(std::move(map));
	}

	static void Load(const std::string& path, const std::wstring& gameFolder)
	{
		g_path = path;
		g_fingerprint = Fingerprint(gameFolder);

		const HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (file == INVALID_HANDLE_VALUE) return;

		LARGE_INTEGER size{};
		std::string text;

		if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < 256ll * 1024 * 1024)
		{
			text.resize(static_cast<size_t>(size.QuadPart));

			DWORD read = 0;
			if (!ReadFile(file, text.data(), static_cast<DWORD>(text.size()), &read, NULL)) read = 0;
			text.resize(read);
		}

		CloseHandle(file);

		std::lock_guard<std::mutex> guard(g_lock);

		size_t cursor = 0;
		bool header = false;
		std::string fingerprint;
		std::unique_ptr<Map> current;

		while (cursor < text.size())
		{
			const size_t end = text.find('\n', cursor);

			// An unfinished line is from a write that never ended
			if (end == std::string::npos) break;

			std::string line = text.substr(cursor, end - cursor);
			cursor = end + 1;

			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty()) continue;

			const char type = line[0];
			const char* rest = line.size() > 2 ? line.c_str() + 2 : "";

			if (!header)
			{
				if (line != "MPSM 1") return;

				header = true;
				continue;
			}

			if (type == 'D')
			{
				fingerprint += line + "\n";
				continue;
			}

			// The archives changed since the maps were learned
			if (!g_fileValid)
			{
				if (fingerprint != g_fingerprint) return;

				g_fileValid = true;
			}

			if (type == 'S')
			{
				char* next = nullptr;
				current = std::make_unique<Map>();
				current->fileSize = static_cast<uint32_t>(strtoul(rest, &next, 10));
				current->hash = strtoull(next, &next, 16);
				while (*next == ' ') next++;
				current->name = next;
			}
			else if (type == 'H' && current)
			{
				const size_t length = strlen(rest);

				for (size_t index = 0; index + 8 <= length; index += 8)
				{
					char digits[9] = {};
					std::memcpy(digits, rest + index, 8);
					current->samples.push_back(static_cast<uint32_t>(strtoul(digits, nullptr, 16)));
				}
			}
			else if (type == 'R' && current)
			{
				char* next = nullptr;
				Range range;
				range.first = static_cast<uint32_t>(strtoul(rest, &next, 10));
				range.count = static_cast<uint32_t>(strtoul(next, &next, 10));
				while (*next == ' ') next++;
				range.key = next;
				current->ranges.push_back(std::move(range));
			}
			else if (type == 'E' && current)
			{
				const uint32_t blocks = current->fileSize / BLOCK_SIZE;
				bool valid = current->fileSize % BLOCK_SIZE == 0 && current->samples.size() == blocks && !current->name.empty();

				for (const Range& range : current->ranges)
				{
					if (range.first == 0 || range.count == 0 || range.first + range.count > blocks) valid = false;
				}

				if (valid) CommitLocked(std::move(current));
				current.reset();
			}
		}

		if (header && fingerprint == g_fingerprint) g_fileValid = true;
	}

	static void AppendLocked(const Map& map)
	{
		std::string text;

		// First write, or the file was learned from other archives
		if (!g_fileValid)
		{
			text = "MPSM 1\n" + g_fingerprint;
		}

		char line[128];
		snprintf(line, sizeof(line), "S %u %016llx ", map.fileSize, static_cast<unsigned long long>(map.hash));
		text += line + map.name + "\n";

		for (size_t index = 0; index < map.samples.size(); index++)
		{
			if (index % 512 == 0) text += index ? "\nH " : "H ";

			snprintf(line, sizeof(line), "%08x", map.samples[index]);
			text += line;
		}

		if (!map.samples.empty()) text += "\n";

		for (const Range& range : map.ranges)
		{
			snprintf(line, sizeof(line), "R %u %u ", range.first, range.count);
			text += line + range.key + "\n";
		}

		text += "E\n";

		const HANDLE file = CreateFileA(g_path.c_str(), g_fileValid ? FILE_APPEND_DATA : GENERIC_WRITE, FILE_SHARE_READ, NULL, g_fileValid ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE) return;

		DWORD written = 0;
		if (WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, NULL) && written == text.size()) g_fileValid = true;

		CloseHandle(file);
	}

	static const Map* Find(const std::string& name, uint32_t fileSize, uint64_t hash)
	{
		std::lock_guard<std::mutex> guard(g_lock);

		const auto entry = g_maps.find(name);

		if (entry == g_maps.end()) return nullptr;

		for (const auto& map : entry->second)
		{
			if (map->fileSize == fileSize && map->hash == hash && !map->invalid) return map.get();
		}

		return nullptr;
	}

	static void Add(std::unique_ptr<Map> map)
	{
		std::lock_guard<std::mutex> guard(g_lock);

		const auto entry = g_maps.find(map->name);

		if (entry != g_maps.end())
		{
			for (const auto& existing : entry->second)
			{
				if (existing->fileSize == map->fileSize && existing->hash == map->hash && existing->samples == map->samples && !existing->invalid) return;
			}
		}

		AppendLocked(*map);
		CommitLocked(std::move(map));
	}

	static void Start(Learner& learner, uint32_t fileSize)
	{
		learner = Learner();

		// The engine only reads whole blocks
		if (fileSize == 0 || fileSize % BLOCK_SIZE != 0) return;

		learner.active = true;
		learner.fileSize = fileSize;
		learner.samples.assign(fileSize / BLOCK_SIZE, 0);
		learner.owners.assign(fileSize / BLOCK_SIZE, -1);
	}

	// True once the last chunk of the stream was seen
	static bool Learn(Learner& learner, uint64_t position, const uint32_t* pChunk, const std::string* headerKey)
	{
		if (!learner.active) return false;

		const uint32_t size = pChunk[1];
		const uint32_t block = static_cast<uint32_t>(position / BLOCK_SIZE);
		const uint32_t offset = static_cast<uint32_t>(position % BLOCK_SIZE);

		if (position != learner.next || size < 8 || offset + size > BLOCK_SIZE || block >= learner.owners.size())
		{
			learner.active = false;
			return false;
		}

		if (offset == 0)
		{
			const uint8_t* data = reinterpret_cast<const uint8_t*>(pChunk);
			learner.samples[block] = SampleBlock(data);

			if (block == 0) learner.hash = HashBlock(data);
		}

		// Blocks were skipped, this pass can't teach anything
		if (pChunk[0] == FOURCC_SKIP)
		{
			learner.active = false;
			return false;
		}

		if (pChunk[0] == FOURCC_SHOC && size >= 12)
		{
			int32_t& owner = learner.owners[block];

			if (pChunk[2] == FOURCC_SHDR)
			{
				owner = -2;
				learner.current = -2;

				if (headerKey && !headerKey->empty())
				{
					const auto inserted = learner.ids.emplace(*headerKey, static_cast<int32_t>(learner.keys.size()));
					if (inserted.second) learner.keys.push_back(*headerKey);
					learner.current = inserted.first->second;
				}
			}
			else if (pChunk[2] == FOURCC_SDAT || pChunk[2] == FOURCC_RPAK)
			{
				if (owner == -1) owner = learner.current;
				else if (owner != learner.current) owner = -2;
			}
		}

		learner.next = position + size;
		return learner.next == learner.fileSize;
	}

	static std::unique_ptr<Map> Finish(const Learner& learner, const std::string& name)
	{
		auto map = std::make_unique<Map>();
		map->name = name;
		map->fileSize = learner.fileSize;
		map->hash = learner.hash;
		map->samples = learner.samples;

		// Block 0 always holds the first header
		for (uint32_t block = 1; block < learner.owners.size(); block++)
		{
			const int32_t owner = learner.owners[block];

			if (owner < 0) continue;

			if (!map->ranges.empty())
			{
				Range& last = map->ranges.back();

				if (last.first + last.count == block && last.key == learner.keys[owner])
				{
					last.count++;
					continue;
				}
			}

			map->ranges.push_back({ block, 1, learner.keys[owner] });
		}

		return map;
	}

	// The first block came off the disk, decides which blocks won't be read
	static void Verify(Reader& reader, const uint8_t* block, uint32_t fileSize)
	{
		reader.map = nullptr;
		reader.skip.clear();

		if (fileSize == 0 || fileSize % BLOCK_SIZE != 0) return;

		const Map* map = Find(reader.name, fileSize, HashBlock(block));

		if (!map || map->samples.empty() || map->samples[0] != SampleBlock(block)) return;

		reader.map = map;
		reader.skip.assign(map->samples.size(), 0);

		// Only what a loose file replaces, the header of these assets is replaced every time the engine reaches it
		for (const Range& range : map->ranges)
		{
			if (!ModFiles::Find(range.key)) continue;

			std::fill(reader.skip.begin() + range.first, reader.skip.begin() + range.first + range.count, 1);
		}
	}

	// A block that is still read must look like the one the map was learned from, otherwise nothing more is skipped
	static void Check(Reader& reader, uint32_t block, const uint8_t* data)
	{
		if (!reader.map) return;

		if (block < reader.map->samples.size() && reader.map->samples[block] == SampleBlock(data)) return;

		reader.map->invalid = true;
		reader.map = nullptr;
		reader.skip.clear();
	}

	static bool ShouldSkip(const Reader& reader, uint32_t block)
	{
		return reader.map && block < reader.skip.size() && reader.skip[block];
	}
}
