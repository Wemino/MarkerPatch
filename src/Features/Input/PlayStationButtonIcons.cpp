#pragma once

#include "../../Globals.cpp"
#include "../../Fonts/PlayStationButtons.hpp"

// ======================
// PlayStationButtonIcons
// ======================

safetyhook::InlineHook TM_DownloadTextureData;
safetyhook::InlineHook FFNFont_Init;
safetyhook::InlineHook FontManager_UnloadResource;

enum TextureFormat
{
	TX_DXT5 = 0x24,
};

struct FontChar
{
	uint8_t m_index[2];
	uint16_t m_width;
	uint16_t m_height;
	uint16_t m_advance;
	int16_t m_xoffset;
	int16_t m_yoffset;
	uint16_t m_u1;
	uint16_t m_v1;
	uint16_t m_u2;
	uint16_t m_v2;
	uint16_t m_numKern;
	uint16_t m_kernIndex;
};

struct FontCharTable
{
	FontChar* m_table[256];
};

struct FFNFont
{
	unsigned int m_id;
	unsigned int m_size;
	uint16_t m_version;
	uint16_t m_num;
	int m_chartbl;
	int m_shape;
	int m_maxX;
	int m_maxY;
	int m_ascent;
	char m_name[32];
	int m_tables;
	FontCharTable* m_table[256];
	unsigned int m_textureHandle;
};

struct TextureHashTableEntry
{
	int referenceCount;
	unsigned int ref;
	int lifetime;
	unsigned int budget;
	unsigned int size;
	uint16_t width;
	uint16_t height;
	uint8_t numLayers;
};

struct CResourceUnloadInfo
{
	unsigned int m_typeID;
	const char* m_pTypeName;
	void* m_pData;
	const uint32_t* m_pGUID;
};

static TextureHashTableEntry*(__cdecl* TH_GetEntry)(unsigned int) = nullptr;
static unsigned int(__cdecl* TM_FindTexture_Deprecated)(const char*) = nullptr;
static bool(__cdecl* TM_GetTextureFormat)(unsigned int, TextureFormat&, unsigned int&, int&) = nullptr;
static unsigned int(__cdecl* TX_FormatSize)(TextureFormat, unsigned int, unsigned int) = nullptr;

static constexpr unsigned int BUTTON_FONT_HASH = 0x320036A1; // "buttons"
static constexpr unsigned int FONT_TYPE_ID = 0x32A046EE;
static constexpr int BUTTON_CHAR_COUNT = 16;

// shared\fonts\buttons of the PlayStation 3 version
static constexpr unsigned int PLAYSTATION_TEXTURE_SIZE = 256;
static constexpr unsigned int PLAYSTATION_TEXTURE_LAYERS = 3;
static constexpr int PLAYSTATION_FONT_MAX_Y = 68;

static constexpr FontChar PLAYSTATION_CHARS[BUTTON_CHAR_COUNT] =
{
	{ { 0, 0 }, 79, 50, 84, 0, 0, 0x0740, 0x03F0, 0x0C30, 0x0710, 0, 0 }, // Start
	{ { 1, 0 }, 91, 52, 102, 0, 0, 0x0410, 0x0080, 0x09C0, 0x03C0, 0, 0 }, // Select
	{ { 2, 0 }, 64, 64, 64, 0, 0, 0x0800, 0x0C00, 0x0C00, 0x1000, 0, 0 }, // LSBottom
	{ { 3, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0C00, 0x0400, 0x1000, 0, 0 }, // LSTop
	{ { 4, 0 }, 64, 64, 64, 6, 4, 0x0C00, 0x0C00, 0x1000, 0x1000, 0, 0 }, // RSBottom
	{ { 5, 0 }, 64, 64, 64, 0, 0, 0x0400, 0x0C00, 0x0800, 0x1000, 0, 0 }, // RSTop
	{ { 6, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0000, 0x0400, 0x0400, 0, 0 }, // LDLeft
	{ { 7, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0000, 0x0400, 0x0400, 0, 0 }, // LDRight
	{ { 8, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0000, 0x0400, 0x0400, 0, 0 }, // LDUp
	{ { 9, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0000, 0x0400, 0x0400, 0, 0 }, // LDDown
	{ { 10, 0 }, 64, 64, 64, 0, 0, 0x0400, 0x0800, 0x0800, 0x0C00, 0, 0 }, // RDLeft
	{ { 11, 0 }, 64, 64, 64, 0, 0, 0x0800, 0x0800, 0x0C00, 0x0C00, 0, 0 }, // RDRight
	{ { 12, 0 }, 64, 64, 64, 0, 0, 0x0000, 0x0800, 0x0400, 0x0C00, 0, 0 }, // RDUp
	{ { 13, 0 }, 64, 64, 64, 0, 0, 0x0C00, 0x0800, 0x1000, 0x0C00, 0, 0 }, // RDDown
	{ { 14, 0 }, 57, 57, 64, 0, 0, 0x0010, 0x0450, 0x03A0, 0x07E0, 0, 0 }, // LStick
	{ { 15, 0 }, 57, 57, 64, 0, 0, 0x03B0, 0x0450, 0x0740, 0x07E0, 0, 0 }, // RStick
};

static FFNFont* g_buttonFont = nullptr;
static FontChar* g_xboxChars[BUTTON_CHAR_COUNT] = {};
static FontChar g_playStationChars[BUTTON_CHAR_COUNT] = {};
static bool g_arePlayStationCharsReady = false;
static bool g_isShowingPlayStation = false;

static TextureHashTableEntry* g_buttonsEntry = nullptr;
static TextureHashTableEntry g_xboxEntry = {};
static TextureHashTableEntry g_combinedEntry = {};
static unsigned int g_xboxTextureHeight = 0;

static unsigned int RoundUpToPowerOfTwo(unsigned int value)
{
	unsigned int result = 1;

	while (result < value)
	{
		result <<= 1;
	}

	return result;
}

static unsigned int GetDXT5Size(unsigned int width, unsigned int height, unsigned int numLayers)
{
	unsigned int size = 0;

	for (unsigned int layer = 0; layer < numLayers; layer++)
	{
		size += TX_FormatSize(TX_DXT5, width >> layer, height >> layer);
	}

	return size;
}

static bool IsSameTextureSize(const TextureHashTableEntry& entry, const TextureHashTableEntry& other)
{
	return entry.size == other.size && entry.width == other.width && entry.height == other.height && entry.numLayers == other.numLayers;
}

static void SetTextureSize(TextureHashTableEntry* entry, const TextureHashTableEntry& from)
{
	entry->size = from.size;
	entry->width = from.width;
	entry->height = from.height;
	entry->numLayers = from.numLayers;
}

// Layer by layer, the block rows of the PC texture then those of the PlayStation one
static std::vector<uint8_t> BuildButtonsTexture(const uint8_t* data)
{
	std::vector<uint8_t> texture(g_combinedEntry.size, 0); // Zeroed DXT5 blocks are transparent

	const uint8_t* xboxLayer = data;
	const uint8_t* playStationLayer = PlayStationButtonsTexture;
	uint8_t* combinedLayer = texture.data();

	for (unsigned int layer = 0; layer < g_combinedEntry.numLayers; layer++)
	{
		const unsigned int combinedRowSize = TX_FormatSize(TX_DXT5, g_combinedEntry.width >> layer, 4);
		const unsigned int xboxRowSize = TX_FormatSize(TX_DXT5, g_xboxEntry.width >> layer, 4);
		const unsigned int xboxRows = (g_xboxEntry.height >> layer) / 4;
		const unsigned int playStationRowSize = TX_FormatSize(TX_DXT5, PLAYSTATION_TEXTURE_SIZE >> layer, 4);
		const unsigned int playStationRows = (PLAYSTATION_TEXTURE_SIZE >> layer) / 4;

		for (unsigned int row = 0; row < xboxRows; row++)
		{
			std::memcpy(combinedLayer + row * combinedRowSize, xboxLayer + row * xboxRowSize, xboxRowSize);
		}

		for (unsigned int row = 0; row < playStationRows; row++)
		{
			std::memcpy(combinedLayer + (xboxRows + row) * combinedRowSize, playStationLayer + row * playStationRowSize, playStationRowSize);
		}

		xboxLayer += xboxRows * xboxRowSize;
		playStationLayer += playStationRows * playStationRowSize;
		combinedLayer += TX_FormatSize(TX_DXT5, g_combinedEntry.width >> layer, g_combinedEntry.height >> layer);
	}

	return texture;
}

static int16_t ScaleToButtonFont(int value)
{
	return static_cast<int16_t>(std::lround(static_cast<float>(value) * g_buttonFont->m_maxY / PLAYSTATION_FONT_MAX_Y));
}

static void PreparePlayStationChars()
{
	if (g_buttonFont == nullptr || g_xboxTextureHeight == 0 || g_buttonFont->m_maxY <= 0) return;

	for (int i = 0; i < BUTTON_CHAR_COUNT; i++)
	{
		FontChar& fontChar = g_playStationChars[i];
		fontChar = PLAYSTATION_CHARS[i];
		fontChar.m_width = ScaleToButtonFont(fontChar.m_width);
		fontChar.m_height = ScaleToButtonFont(fontChar.m_height);
		fontChar.m_advance = ScaleToButtonFont(fontChar.m_advance);
		fontChar.m_xoffset = ScaleToButtonFont(fontChar.m_xoffset);
		fontChar.m_yoffset = ScaleToButtonFont(fontChar.m_yoffset);
		fontChar.m_v1 += static_cast<uint16_t>(g_xboxTextureHeight * 16);
		fontChar.m_v2 += static_cast<uint16_t>(g_xboxTextureHeight * 16);
	}

	g_arePlayStationCharsReady = true;
}

static bool __cdecl TM_DownloadTextureData_Hook(unsigned int ref, void* data, unsigned int dataSize, bool& bTexDownloadIssued)
{
	TextureHashTableEntry* entry = TH_GetEntry(ref);
	const bool isButtonFontTexture = ref == TM_FindTexture_Deprecated("buttons") || (g_buttonFont != nullptr && ref == g_buttonFont->m_textureHandle);

	if (entry == nullptr || data == nullptr || !isButtonFontTexture)
	{
		return TM_DownloadTextureData.unsafe_ccall<bool>(ref, data, dataSize, &bTexDownloadIssued);
	}

	// Already enlarged by an earlier download
	if (entry == g_buttonsEntry && IsSameTextureSize(*entry, g_combinedEntry))
	{
		SetTextureSize(entry, g_xboxEntry);
	}

	TextureFormat format = static_cast<TextureFormat>(0);
	unsigned int options = 0;
	int lodBias = 0;

	// Every layer must start the PlayStation texture on a block row
	unsigned int numLayers = std::min(static_cast<unsigned int>(entry->numLayers), PLAYSTATION_TEXTURE_LAYERS);

	while (numLayers > 0 && (entry->width % (4u << (numLayers - 1)) != 0 || entry->height % (4u << (numLayers - 1)) != 0))
	{
		numLayers--;
	}

	if (numLayers == 0 || !TM_GetTextureFormat(ref, format, options, lodBias) || format != TX_DXT5 || (options & 6) != 0 || dataSize < GetDXT5Size(entry->width, entry->height, numLayers))
	{
		return TM_DownloadTextureData.unsafe_ccall<bool>(ref, data, dataSize, &bTexDownloadIssued);
	}

	g_buttonsEntry = entry;
	g_xboxEntry = *entry;
	g_combinedEntry.width = static_cast<uint16_t>(RoundUpToPowerOfTwo(std::max(static_cast<unsigned int>(entry->width), PLAYSTATION_TEXTURE_SIZE)));
	g_combinedEntry.height = static_cast<uint16_t>(RoundUpToPowerOfTwo(entry->height + PLAYSTATION_TEXTURE_SIZE));
	g_combinedEntry.numLayers = static_cast<uint8_t>(numLayers);
	g_combinedEntry.size = GetDXT5Size(g_combinedEntry.width, g_combinedEntry.height, numLayers);

	std::vector<uint8_t> texture = BuildButtonsTexture(static_cast<const uint8_t*>(data));
	SetTextureSize(entry, g_combinedEntry);

	if (!TM_DownloadTextureData.unsafe_ccall<bool>(ref, texture.data(), g_combinedEntry.size, &bTexDownloadIssued))
	{
		SetTextureSize(entry, g_xboxEntry);
		return TM_DownloadTextureData.unsafe_ccall<bool>(ref, data, dataSize, &bTexDownloadIssued);
	}

	g_xboxTextureHeight = g_xboxEntry.height;
	PreparePlayStationChars();
	return true;
}

static FFNFont* __cdecl FFNFont_Init_Hook(void* pvFontData, unsigned int uHashID, unsigned int textureHandle)
{
	FFNFont* font = FFNFont_Init.unsafe_ccall<FFNFont*>(pvFontData, uHashID, textureHandle);

	if (uHashID != BUTTON_FONT_HASH || font == nullptr || font->m_table[0] == nullptr) return font;

	std::memcpy(g_xboxChars, font->m_table[0]->m_table, sizeof(g_xboxChars));
	g_buttonFont = font;
	g_isShowingPlayStation = false;
	PreparePlayStationChars();
	return font;
}

static void __fastcall FontManager_UnloadResource_Hook(uintptr_t thisp, int, CResourceUnloadInfo& unloadInfo)
{
	if (unloadInfo.m_typeID == FONT_TYPE_ID && unloadInfo.m_pData == g_buttonFont)
	{
		g_buttonFont = nullptr;
		g_arePlayStationCharsReady = false;
		g_isShowingPlayStation = false;
	}

	FontManager_UnloadResource.unsafe_thiscall<void>(thisp, &unloadInfo);
}

static void UpdatePlayStationButtonIcons()
{
	if (!g_arePlayStationCharsReady) return;

	const bool showPlayStation = PlayStationButtonIcons == 1 || (PlayStationButtonIcons == 2 && ControllerHelper::GetGamepadStyle() == ControllerHelper::GamepadStyle::PlayStation);

	if (showPlayStation == g_isShowingPlayStation) return;

	for (int i = 0; i < BUTTON_CHAR_COUNT; i++)
	{
		g_buttonFont->m_table[0]->m_table[i] = showPlayStation ? &g_playStationChars[i] : g_xboxChars[i];
	}

	g_isShowingPlayStation = showPlayStation;
}

static void ApplyPlayStationButtonIcons()
{
	if (!PlayStationButtonIcons) return;

	TH_GetEntry = reinterpret_cast<decltype(TH_GetEntry)>(GetAddress(Addr::TH_GetEntry));
	TM_FindTexture_Deprecated = reinterpret_cast<decltype(TM_FindTexture_Deprecated)>(GetAddress(Addr::TM_FindTexture_Deprecated));
	TM_GetTextureFormat = reinterpret_cast<decltype(TM_GetTextureFormat)>(GetAddress(Addr::TM_GetTextureFormat));
	TX_FormatSize = reinterpret_cast<decltype(TX_FormatSize)>(GetAddress(Addr::TX_FormatSize));

	TM_DownloadTextureData = HookHelper::CreateHook((void*)GetAddress(Addr::TM_DownloadTextureData), &TM_DownloadTextureData_Hook);
	FFNFont_Init = HookHelper::CreateHook((void*)GetAddress(Addr::FFNFont_Init), &FFNFont_Init_Hook);
	FontManager_UnloadResource = HookHelper::CreateHook((void*)GetAddress(Addr::FontManager_UnloadResource), &FontManager_UnloadResource_Hook);
}
