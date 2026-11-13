#pragma once

#include "../../Globals.cpp"
#include "../../Movies/FE08Locker.hpp"

// =========================
// MarkerPatchMenu
// =========================

safetyhook::InlineHook LoadAPX_AllocateBufferAndSeparateTextures;
safetyhook::InlineHook FE08Locker_Constructor;
safetyhook::InlineHook FE08Locker_Init;
safetyhook::InlineHook FE08Locker_HandleInput;
safetyhook::InlineHook FE08Locker_OnInput_Select;
safetyhook::InlineHook FE08Locker_OnInput_Cancel;
safetyhook::InlineHook FE08Locker_DoUpdateItemAndLabel;
safetyhook::InlineHook UIScrollBar_ScrollBy;
safetyhook::InlineHook UIMenuScreenBase_OnHotSpotPressed;
safetyhook::InlineHook UIHotSpotScreen_AddHelpButtons;

enum eAptHandleInput
{
	eAptHandleInput_NotHandled,
	eAptHandleInput_PassToNext,
	eAptHandleInput_Handled,
	eAptHandleInput_HandledTripAuto,
	eAptHandleInput_HandledForceSlowestAuto,
};

enum VTType
{
	eValueType_int32,
	eValueType_uint32,
	eValueType_bool,
	eValueType_char,
	eValueType_float,
	eValueType_char_static,
	eValueType_char_readonly,
};

struct GPOPParams
{
	enum Button
	{
		BUTTON_NONE,
		BUTTON_START,
		BUTTON_ADVANCE,
		BUTTON_BACK,
		BUTTON_OTHER1,
		BUTTON_OTHER2,
	};

	enum Style
	{
		STYLE_NORMAL,
		STYLE_SAVE_MESSAGE,
	};

	enum
	{
		PRIORITY_NORMAL = 1,
		PRIORITY_HIGHEST = 3,
	};

	struct Option
	{
		const char* text;
		Button button;
	};

	const char* titleString;
	const char* messageString;
	bool bCanBeOverriden;
	unsigned int priority;
	Style style;
	Option m_optionList[4];
};

struct GPOPResult
{
	unsigned int popupHandle;
	bool bCancelled;
	int optionIndex;
	GPOPParams::Button optionButton;
};

struct FE08MenuItem
{
	const char* mLabel;
	const char* mName;
};

struct CMsg
{
	unsigned int Id;
	bool broadcast;
	uint32_t m_handle;
};

static bool(__thiscall* AptSubscriberVariable_AddVariable)(uintptr_t, const char*, void*, VTType, void*, void*, bool) = nullptr;
static void(__cdecl* AptCallFunction)(const char*, char*, const char*, int, ...) = nullptr;
static int(__thiscall* UIExternScrollMenuIterator_ActiveItemIndex)(uintptr_t) = nullptr;
static int(__thiscall* UIExternScrollMenuIterator_ActivateItem)(uintptr_t, int) = nullptr;
static void(__thiscall* UIExternScrollMenuIterator_UpdateItem)(uintptr_t, int, bool) = nullptr;
static uintptr_t(__thiscall* UIMenuScreenBase_GetScrollBar)(uintptr_t) = nullptr;
static void(__thiscall* UIHotSpotScreen_AddHelpButton)(uintptr_t, uint32_t, uint32_t, int) = nullptr;
static void(__thiscall* UIHotSpotScreen_UpdateHelpButtons)(uintptr_t) = nullptr;
static unsigned int(__thiscall* GPOPManager_ShowPopup)(uintptr_t, const GPOPParams*, uint64_t, uint64_t, uint64_t) = nullptr;
static bool(__cdecl* RWS__SendMsg)(CMsg*, bool) = nullptr;

// shared\ui\apx\fe08locker_pc.apx
static constexpr uint32_t LOCKER_MOVIE_SIZE = 132304;
static constexpr uint64_t LOCKER_MOVIE_HASH = 0xAA8BBCDE0A2E1AB7;

static constexpr int MAX_ROWS = 7;
static constexpr int LOCKER_ROWS = 3;
static constexpr int MARKERPATCH_ROW = 2;

static constexpr int INPUT_LEFT = 6;
static constexpr int INPUT_RIGHT = 7;
static constexpr int INPUT_UP = 8;
static constexpr int INPUT_DOWN = 9;
static constexpr int INPUT_DEFAULTS = 10;
static constexpr int INPUT_SELECT = 13;

static constexpr int HOT_SPOT_MENU_ITEM = 0;
static constexpr int HOT_SPOT_HELP_BUTTON = 6;

static constexpr uint32_t HELP_BUTTON_DEFAULTS = 0x64656661; // 'defa'
static constexpr uint32_t HELP_BUTTON_ACCEPT = 0x61637074; // 'acpt'

enum MenuPage
{
	PAGE_LOCKER,
	PAGE_MARKERPATCH,
	PAGE_DLC,
	PAGE_GRAPHICS,
	PAGE_CONTROLLER,
	PAGE_MISC,
};

enum class OptionType
{
	Toggle,
	Choice,
	Number,
	Decimal,
};

enum class OptionApply
{
	Live,
	Restart,
	NextLaunch,
	HookedAtLaunch,
};

struct MenuOption
{
	const char* label;
	const char* section;
	const char* key;
	OptionType type;
	void* setting;
	int defaultValue;
	OptionApply apply;
	void (*onChange)(const MenuOption&) = nullptr;
	const char* const* names = nullptr;
	const int* numbers = nullptr;
	int count = 0;
	int launchValue = 0;
	int savedValue = 0;
	int value = 0;
};

struct MenuPageInfo
{
	const char* name;
	const char* title;
	MenuOption* options;
	int optionCount;
};

enum class MenuRequest
{
	None,
	Restart,
	PatchExecutable,
	DiscardChanges,
};

static void OnContentPackChanged(const MenuOption& option)
{
	UpdateContentPack(static_cast<const bool*>(option.setting));
}

static void OnGyroChanged(const MenuOption&)
{
	ControllerHelper::SetGyroEnabled(GyroEnabled);
	ControllerHelper::SetGyroSensitivity(GyroSensitivity);
	ControllerHelper::SetGyroInvertX(GyroInvertX);
	ControllerHelper::SetGyroInvertY(GyroInvertY);
}

static const char* const ToggleNames[] = { "off", "on" };
static const char* const AimAccelerationNames[] = { "on", "off" };
static const char* const AntiAliasingNames[] = { "off", "fxaa", "smaa", "ssaa", "smaa + ssaa" };
static const int ShadowResolutions[] = { 1920, 2560, 3072, 4096 };
static const int GyroSensitivityRange[] = { 10, 400, 10 };

static const char* const MultiplayerSuitNames[] =
{
	"off",
	"solid blue", "solid army green", "solid red", "solid black", "solid olive",
	"urban blue", "urban army green", "urban red", "urban military", "urban olive",
	"tiger blue", "tiger army green", "tiger red", "tiger military",
	"war torn", "visceral", "collector's",
	"dr pepper maroon", "dr pepper urban", "dr pepper tiger",
	"viral blue", "viral urban blue", "viral tiger blue",
	"viral yellow", "viral urban yellow", "viral tiger yellow",
	"viral white", "viral urban white", "viral tiger white",
	"matchett", "gascoigne", "probst", "woldman", "papoutsis", "murphy", "lu",
};

static MenuOption DLCOptions[] =
{
	{ "hazard pack", "DLC", "EnableHazardPack", OptionType::Toggle, &EnableHazardPack, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "martial law pack", "DLC", "EnableMartialLawPack", OptionType::Toggle, &EnableMartialLawPack, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "supernova pack", "DLC", "EnableSupernovaPack", OptionType::Toggle, &EnableSupernovaPack, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "severed items", "DLC", "EnableSeveredDLC", OptionType::Toggle, &EnableSeveredDLC, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "hacker items", "DLC", "EnableHackerDLC", OptionType::Toggle, &EnableHackerDLC, 0, OptionApply::HookedAtLaunch, nullptr, ToggleNames },
	{ "zealot items", "DLC", "EnableZealotDLC", OptionType::Toggle, &EnableZealotDLC, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "rivet gun", "DLC", "EnableRivetGunDLC", OptionType::Toggle, &EnableRivetGunDLC, 0, OptionApply::Live, OnContentPackChanged, ToggleNames },
	{ "ignition rooms", "DLC", "EnableIgnitionRooms", OptionType::Toggle, &EnableIgnitionRooms, 0, OptionApply::Restart, nullptr, ToggleNames },
	{ "original cutter", "DLC", "EnableOriginalPlasmaCutter", OptionType::Toggle, &EnableOriginalPlasmaCutter, 0, OptionApply::Restart, nullptr, ToggleNames },
	{ "mp suit", "DLC", "MultiplayerSuit", OptionType::Choice, &MultiplayerSuit, 0, OptionApply::HookedAtLaunch, nullptr, MultiplayerSuitNames, nullptr, static_cast<int>(std::size(MultiplayerSuitNames)) },
};

static MenuOption GraphicsOptions[] =
{
	{ "very high shadow resolution", "Graphics", "DynamicShadowResolution", OptionType::Number, &DynamicShadowResolution, 1920, OptionApply::Restart, nullptr, nullptr, ShadowResolutions, static_cast<int>(std::size(ShadowResolutions)) },
	{ "anti-aliasing", "Graphics", "ImprovedAntiAliasingMode", OptionType::Choice, &ImprovedAntiAliasingMode, 2, OptionApply::Restart, nullptr, AntiAliasingNames, nullptr, static_cast<int>(std::size(AntiAliasingNames)) },
};

static MenuOption ControllerOptions[] =
{
	{ "gyro aiming", "Input", "GyroEnabled", OptionType::Toggle, &GyroEnabled, 0, OptionApply::Live, OnGyroChanged, ToggleNames },
	{ "gyro sensitivity", "Input", "GyroSensitivity", OptionType::Decimal, &GyroSensitivity, 100, OptionApply::Live, OnGyroChanged, nullptr, GyroSensitivityRange },
	{ "gyro invert x", "Input", "GyroInvertX", OptionType::Toggle, &GyroInvertX, 0, OptionApply::Live, OnGyroChanged, ToggleNames },
	{ "gyro invert y", "Input", "GyroInvertY", OptionType::Toggle, &GyroInvertY, 0, OptionApply::Live, OnGyroChanged, ToggleNames },
	{ "aim acceleration", "Input", "DisableControllerAimAcceleration", OptionType::Toggle, &DisableControllerAimAcceleration, 0, OptionApply::HookedAtLaunch, nullptr, AimAccelerationNames },
};

static MenuOption MiscOptions[] =
{
	{ "achievements", "General", "AchievementSupport", OptionType::Toggle, &AchievementSupport, 1, OptionApply::Restart, nullptr, ToggleNames },
	{ "skip intro", "General", "SkipIntro", OptionType::Toggle, &SkipIntro, 0, OptionApply::NextLaunch, nullptr, ToggleNames },
};

static MenuPageInfo MenuPages[] =
{
	{ nullptr, "$fe_top_locker", nullptr, 0 },
	{ "markerpatch", "//markerpatch", nullptr, 0 },
	{ "dlc", "//dlc", DLCOptions, static_cast<int>(std::size(DLCOptions)) },
	{ "graphics", "//graphics", GraphicsOptions, static_cast<int>(std::size(GraphicsOptions)) },
	{ "controller", "//controller", ControllerOptions, static_cast<int>(std::size(ControllerOptions)) },
	{ "misc", "//misc", MiscOptions, static_cast<int>(std::size(MiscOptions)) },
};

static constexpr int FIRST_OPTION_PAGE = PAGE_DLC;
static constexpr int OPTION_PAGE_COUNT = PAGE_MISC - PAGE_DLC + 1;
static constexpr int LAA_PATCH_ENTRY = OPTION_PAGE_COUNT;

static constexpr const char* LAA_PATCH_MESSAGE = "Patches the game's executable so that it can use up to 4 GB of memory instead of 2 GB, "
"which can improve performance. A backup of the original file is kept. The game restarts to apply it, continue?";

// shared\fonts\russellsquare32
static const uint8_t RussellSquareAdvances[] =
{
	9, 12, 14, 27, 28, 33, 27, 9, 13, 13, 21, 25, 13, 14, 13, 12,
	27, 13, 26, 24, 26, 27, 27, 22, 27, 27, 13, 13, 25, 25, 25, 22,
	34, 29, 29, 27, 29, 25, 24, 28, 27, 12, 22, 24, 20, 36, 29, 29,
	27, 29, 29, 29, 24, 28, 24, 42, 27, 24, 27, 13, 12, 13, 25, 21,
	11, 24, 24, 23, 24, 24, 13, 24, 24, 11, 11, 24, 11, 37, 24, 24,
	24, 24, 14, 24, 13, 24, 20, 37, 21, 24, 22, 13, 9, 13, 25,
};

static std::vector<uint8_t> g_lockerMovie;
static bool g_isLockerMovieLoaded = false;
static bool g_isLockerMovieReplaced = false;
static uintptr_t g_locker = 0;

// Read by the movie
static const char* g_menuTitle = "$fe_top_locker";
static const char* g_rowValue = "";
static int g_rowValueWidth = 0;
static int g_rowCount = LOCKER_ROWS;
static int g_firstEntry = 0;
static int g_entryCount = LOCKER_ROWS;
static int g_scrollPercent = 0;

static MenuPage g_page = PAGE_LOCKER;
static uintptr_t g_scrollBar = 0;
static bool g_isRestartPending = false;
static MenuRequest g_menuRequest = MenuRequest::None;
static std::string g_popupMessage;

static uint64_t HashMovie(const uint8_t* data, size_t size)
{
	uint64_t hash = 0xCBF29CE484222325ull;

	for (size_t i = 0; i < size; i++)
	{
		hash = (hash ^ data[i]) * 0x100000001B3ull;
	}

	return hash;
}

static bool IsLockerMovie(const uint8_t* data, uint32_t size)
{
	return size > 0x20 && _strnicmp(reinterpret_cast<const char*>(data + 0x10), "FE08Locker", 11) == 0;
}

static bool IsVanillaLockerMovie(const uint8_t* data, uint32_t size)
{
	if (size != LOCKER_MOVIE_SIZE || !IsLockerMovie(data, size)) return false;

	// Skip the title chunk
	uint32_t titleEnd = 8 + *reinterpret_cast<const uint32_t*>(data + 0xC);
	if (titleEnd >= size) return false;

	return HashMovie(data + titleEnd, size - titleEnd) == LOCKER_MOVIE_HASH;
}

static bool IsMenuActive()
{
	return g_isLockerMovieReplaced && g_page != PAGE_LOCKER;
}

static bool IsOptionPageActive()
{
	return g_isLockerMovieReplaced && g_page >= FIRST_OPTION_PAGE;
}

static int GetTextWidth(const char* text)
{
	int width = 0;

	for (; *text != '\0'; text++)
	{
		if (*text >= 0x20 && *text <= 0x7E)
		{
			width += RussellSquareAdvances[*text - 0x20];
		}
	}

	return (width * 24 + 40) / 41;
}

static int GetOptionSetting(const MenuOption& option)
{
	switch (option.type)
	{
		case OptionType::Toggle: return *static_cast<const bool*>(option.setting) ? 1 : 0;
		case OptionType::Decimal: return static_cast<int>(std::lround(*static_cast<const float*>(option.setting) * 100.0f));
		default: return *static_cast<const int*>(option.setting);
	}
}

static void SetOptionSetting(const MenuOption& option)
{
	switch (option.type)
	{
		case OptionType::Toggle: *static_cast<bool*>(option.setting) = option.value != 0; break;
		case OptionType::Decimal: *static_cast<float*>(option.setting) = option.value / 100.0f; break;
		default: *static_cast<int*>(option.setting) = option.value; break;
	}
}

static bool IsAppliedLive(const MenuOption& option)
{
	return option.apply == OptionApply::Live || (option.apply == OptionApply::HookedAtLaunch && option.launchValue != 0);
}

static bool NeedsRestart(const MenuOption& option)
{
	if (option.savedValue == option.launchValue) return false;
	return option.apply == OptionApply::Restart || (option.apply == OptionApply::HookedAtLaunch && option.launchValue == 0);
}

static bool IsRestartNeeded()
{
	for (int page = FIRST_OPTION_PAGE; page < FIRST_OPTION_PAGE + OPTION_PAGE_COUNT; page++)
	{
		for (int i = 0; i < MenuPages[page].optionCount; i++)
		{
			if (NeedsRestart(MenuPages[page].options[i])) return true;
		}
	}

	return false;
}

static void FormatDecimal(char* text, size_t size, int hundredths)
{
	snprintf(text, size, "%d.%02d", hundredths / 100, hundredths % 100);

	// 1.00 -> 1.0
	size_t length = strlen(text);
	if (text[length - 1] == '0') text[length - 1] = '\0';
}

static const char* GetOptionText(const MenuOption& option)
{
	static char text[16];

	switch (option.type)
	{
		case OptionType::Toggle:
			return option.names[option.value != 0 ? 1 : 0];

		case OptionType::Choice:
			return option.value >= 0 && option.value < option.count ? option.names[option.value] : "?";

		case OptionType::Number:
			snprintf(text, sizeof(text), "%d", option.value);
			return text;

		case OptionType::Decimal:
			FormatDecimal(text, sizeof(text), option.value);
			return text;
	}

	return "";
}

static int GetNextOptionValue(const MenuOption& option, int direction, bool wrap)
{
	int value = option.value;

	switch (option.type)
	{
		case OptionType::Toggle:
			return value != 0 ? 0 : 1;

		case OptionType::Choice:
			value += direction;
			if (value < 0) return wrap ? option.count - 1 : 0;
			if (value >= option.count) return wrap ? 0 : option.count - 1;
			return value;

		case OptionType::Number:
			if (direction > 0)
			{
				for (int i = 0; i < option.count; i++)
				{
					if (option.numbers[i] > value) return option.numbers[i];
				}

				return wrap ? option.numbers[0] : option.numbers[option.count - 1];
			}

			for (int i = option.count - 1; i >= 0; i--)
			{
				if (option.numbers[i] < value) return option.numbers[i];
			}

			return wrap ? option.numbers[option.count - 1] : option.numbers[0];

		case OptionType::Decimal:
		{
			int minimum = option.numbers[0];
			int maximum = option.numbers[1];
			int step = option.numbers[2];

			value = direction > 0 ? (value / step + 1) * step : ((value + step - 1) / step - 1) * step;
			if (value > maximum) return wrap ? minimum : maximum;
			if (value < minimum) return wrap ? maximum : minimum;
			return value;
		}
	}

	return value;
}

static void WriteOption(const MenuOption& option)
{
	char text[16];

	if (option.type == OptionType::Decimal)
	{
		FormatDecimal(text, sizeof(text), option.value);
	}
	else
	{
		snprintf(text, sizeof(text), "%d", option.value);
	}

	IniHelper::WriteString(option.section, option.key, text);
}

static void AcceptOptions(MenuPage page)
{
	bool isChanged = false;

	for (int i = 0; i < MenuPages[page].optionCount; i++)
	{
		MenuOption& option = MenuPages[page].options[i];
		if (option.value == option.savedValue) continue;

		option.savedValue = option.value;
		isChanged = true;

		if (IsAppliedLive(option))
		{
			SetOptionSetting(option);

			if (option.onChange)
			{
				option.onChange(option);
			}
		}

		WriteOption(option);

		if (option.apply == OptionApply::Restart || option.apply == OptionApply::HookedAtLaunch)
		{
			g_isRestartPending = true;
		}
	}

	if (isChanged)
	{
		IniHelper::Save();
	}
}

static bool AreOptionsChanged(MenuPage page)
{
	for (int i = 0; i < MenuPages[page].optionCount; i++)
	{
		const MenuOption& option = MenuPages[page].options[i];
		if (option.value != option.savedValue) return true;
	}

	return false;
}

static void CancelOptions(MenuPage page)
{
	for (int i = 0; i < MenuPages[page].optionCount; i++)
	{
		MenuOption& option = MenuPages[page].options[i];
		option.value = option.savedValue;
	}
}

static void ResetOptions(MenuPage page)
{
	for (int i = 0; i < MenuPages[page].optionCount; i++)
	{
		MenuOption& option = MenuPages[page].options[i];
		option.value = option.defaultValue;
	}
}

static void ShowPopup(const char* titleString, const char* messageString, bool askYesNo, void(__cdecl* popupResultFunction)(const GPOPResult*))
{
	uintptr_t popupManager = *reinterpret_cast<uintptr_t*>(g_Addresses.GPOPManagerPtr);
	if (popupManager == 0) return;

	GPOPParams params = {};
	params.titleString = titleString;
	params.messageString = messageString;
	params.bCanBeOverriden = true;
	params.priority = GPOPParams::PRIORITY_NORMAL;

	if (askYesNo)
	{
		params.m_optionList[0] = { "$UIPopup_Yes", GPOPParams::BUTTON_ADVANCE };
		params.m_optionList[1] = { "$UIPopup_No", GPOPParams::BUTTON_BACK };
	}
	else
	{
		params.m_optionList[0] = { "$ui_ok_uc", GPOPParams::BUTTON_ADVANCE };
	}

	GPOPManager_ShowPopup(popupManager, &params, reinterpret_cast<uintptr_t>(popupResultFunction), 0, 0);
}

static void ShowErrorPopup(const char* titleString, const std::wstring& error)
{
	g_popupMessage.clear();

	for (wchar_t c : error)
	{
		g_popupMessage += c == L'\n' ? ' ' : (c < 0x80 ? static_cast<char>(c) : '?');
	}

	ShowPopup(titleString, g_popupMessage.c_str(), false, nullptr);
}

static bool IsPopupAccepted(const GPOPResult* result)
{
	return !result->bCancelled && result->optionButton == GPOPParams::BUTTON_ADVANCE;
}

static void __cdecl OnRestartPopupResult(const GPOPResult* result)
{
	if (IsPopupAccepted(result))
	{
		g_menuRequest = MenuRequest::Restart;
	}
}

static void __cdecl OnLAAPatchPopupResult(const GPOPResult* result)
{
	if (IsPopupAccepted(result))
	{
		g_menuRequest = MenuRequest::PatchExecutable;
	}
}

static void __cdecl OnDiscardPopupResult(const GPOPResult* result)
{
	if (IsPopupAccepted(result))
	{
		g_menuRequest = MenuRequest::DiscardChanges;
	}
}

static void RestartGame()
{
	std::wstring error;
	fs::path exePath;

	LAAPatcher::g_errorOutput = &error;
	bool hasPath = LAAPatcher::GetExecutablePath(GetModuleHandleA(NULL), exePath);
	LAAPatcher::g_errorOutput = nullptr;

	if (hasPath)
	{
		LAAPatcher::RestartGame(exePath);
	}

	ShowPopup("restart", "The game couldn't be restarted, please restart it to apply the changes.", false, nullptr);
}

static bool IsGameLargeAddressAware()
{
	auto base = reinterpret_cast<const uint8_t*>(GetModuleHandleA(NULL));
	auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
	return (ntHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

static void PatchExecutable()
{
	std::wstring error;
	fs::path exePath;
	std::vector<uint8_t> buffer;

	LAAPatcher::g_errorOutput = &error;
	bool isPatched = LAAPatcher::GetExecutablePath(GetModuleHandleA(NULL), exePath) && LAAPatcher::ReadExecutable(exePath, buffer) && (LAAPatcher::PEFile(buffer).IsLAAEnabled() || (LAAPatcher::HasSpaceForPatch(exePath, buffer.size()) && LAAPatcher::PatchExecutable(exePath, buffer)));
	LAAPatcher::g_errorOutput = nullptr;

	if (!isPatched)
	{
		ShowErrorPopup("apply laa patch", error);
		return;
	}

	RestartGame();
}

static void SelectLAAPatch()
{
	std::wstring error;
	fs::path exePath;
	std::vector<uint8_t> buffer;

	LAAPatcher::g_errorOutput = &error;
	bool isRead = LAAPatcher::GetExecutablePath(GetModuleHandleA(NULL), exePath) && LAAPatcher::ReadExecutable(exePath, buffer);
	LAAPatcher::g_errorOutput = nullptr;

	if (!isRead)
	{
		ShowErrorPopup("apply laa patch", error);
		return;
	}

	if (LAAPatcher::PEFile(buffer).IsLAAEnabled())
	{
		ShowPopup("apply laa patch", "The game's executable is already patched, it applies after restarting the game. Restart now?", true, OnRestartPopupResult);
		return;
	}

	ShowPopup("apply laa patch", LAA_PATCH_MESSAGE, true, OnLAAPatchPopupResult);
}

static int GetEntryCount(MenuPage page)
{
	if (page == PAGE_LOCKER) return LOCKER_ROWS;
	if (page == PAGE_MARKERPATCH) return IsGameLargeAddressAware() ? OPTION_PAGE_COUNT : OPTION_PAGE_COUNT + 1;
	return MenuPages[page].optionCount;
}

static MenuOption* GetEntryOption(int entry)
{
	if (g_page < FIRST_OPTION_PAGE || entry < 0 || entry >= MenuPages[g_page].optionCount) return nullptr;
	return &MenuPages[g_page].options[entry];
}

static const char* GetEntryName(int entry)
{
	if (g_page == PAGE_MARKERPATCH)
	{
		return entry == LAA_PATCH_ENTRY ? "apply laa patch" : MenuPages[FIRST_OPTION_PAGE + entry].name;
	}

	MenuOption* option = GetEntryOption(entry);
	return option ? option->label : "";
}

static const char* GetEntryValue(int entry)
{
	MenuOption* option = GetEntryOption(entry);
	return option ? GetOptionText(*option) : "";
}

static void UpdateRows(uintptr_t locker)
{
	for (int row = 0; row < g_rowCount; row++)
	{
		UIExternScrollMenuIterator_UpdateItem(locker + 0x5C, row, true);
	}
}

// The menu only reads mNumSrcElems when the screen opens
static void SetNbrElems(uintptr_t locker, int count)
{
	*reinterpret_cast<int*>(locker + 0x220) = count;

	uintptr_t externArray = *reinterpret_cast<uintptr_t*>(locker + 0x5C + 0x4);
	if (externArray == 0) return;

	auto vSetNbrElems = reinterpret_cast<void(__thiscall*)(uintptr_t, int)>((*reinterpret_cast<uintptr_t**>(externArray))[2]);
	vSetNbrElems(externArray, count);
}

static void SendSoundMsg(DWORD eventIdPtr)
{
	CMsg msg = { *reinterpret_cast<unsigned int*>(eventIdPtr), false, 0 };
	RWS__SendMsg(&msg, false);
}

static int GetScrollRange()
{
	return std::max(g_entryCount - g_rowCount, 0);
}

static void UpdateScrollPercent()
{
	int range = GetScrollRange();
	g_scrollPercent = range > 0 ? g_firstEntry * 100 / range : 0;
}

static void ScrollTo(uintptr_t locker, int first)
{
	first = std::clamp(first, 0, GetScrollRange());
	if (first == g_firstEntry) return;

	g_firstEntry = first;
	UpdateRows(locker);
	SendSoundMsg(g_Addresses.iMsgPlaySoundMenuNavigatePtr);
}

static void __fastcall OnScrollBarDragged(uintptr_t scrollBar, int, int percent)
{
	if (scrollBar != g_scrollBar || !IsMenuActive()) return;

	ScrollTo(g_locker, (GetScrollRange() * percent + 50) / 100);
	AptCallFunction("MPScroll", nullptr, "FE08", 0);
}

static void SetUpScrollBar(uintptr_t locker)
{
	if (g_scrollBar == 0)
	{
		if (GetScrollRange() == 0) return;

		g_scrollBar = UIMenuScreenBase_GetScrollBar(locker);
		if (g_scrollBar == 0) return;

		*reinterpret_cast<int**>(g_scrollBar + 0x74) = &g_scrollPercent;
		*reinterpret_cast<uintptr_t*>(g_scrollBar + 0xA0) = g_scrollBar;
		*reinterpret_cast<void**>(g_scrollBar + 0xA4) = reinterpret_cast<void*>(&OnScrollBarDragged);
	}

	*reinterpret_cast<int*>(g_scrollBar + 0x88) = g_entryCount;
	*reinterpret_cast<int*>(g_scrollBar + 0x90) = g_rowCount;
}

static void ShowPage(uintptr_t locker, MenuPage page, int entry)
{
	g_page = page;
	g_menuTitle = MenuPages[page].title;
	g_entryCount = GetEntryCount(page);
	g_rowCount = std::min(g_entryCount, MAX_ROWS);
	g_firstEntry = std::clamp(entry - g_rowCount + 1, 0, g_entryCount - g_rowCount);
	UpdateScrollPercent();

	SetNbrElems(locker, g_rowCount);
	SetUpScrollBar(locker);
	AptCallFunction("MPLayout", nullptr, "FE08", 0);
	UIHotSpotScreen_UpdateHelpButtons(locker + 0x160);

	UIExternScrollMenuIterator_ActivateItem(locker + 0x5C, entry - g_firstEntry);
	UpdateRows(locker);
}

static void AcceptPage(uintptr_t locker)
{
	AcceptOptions(g_page);
	SendSoundMsg(g_Addresses.iMsgPlaySoundMenuSelectPtr);
	ShowPage(locker, PAGE_MARKERPATCH, g_page - FIRST_OPTION_PAGE);
}

static void ResetPage(uintptr_t locker)
{
	ResetOptions(g_page);
	SendSoundMsg(g_Addresses.iMsgPlaySoundMenuSelectPtr);
	UpdateRows(locker);
}

static void CancelPage(uintptr_t locker)
{
	CancelOptions(g_page);
	ShowPage(locker, PAGE_MARKERPATCH, g_page - FIRST_OPTION_PAGE);
}

static void UpdateMarkerPatchMenu()
{
	if (g_menuRequest == MenuRequest::None) return;

	MenuRequest request = g_menuRequest;
	g_menuRequest = MenuRequest::None;

	if (request == MenuRequest::Restart)
	{
		RestartGame();
	}
	else if (request == MenuRequest::PatchExecutable)
	{
		PatchExecutable();
	}
	else if (request == MenuRequest::DiscardChanges && IsOptionPageActive())
	{
		CancelPage(g_locker);
	}
}

static bool ScrollPage(uintptr_t locker, int row, int direction)
{
	if (g_entryCount <= g_rowCount) return false;

	uintptr_t menu = locker + 0x5C;

	if (direction > 0 && row == g_rowCount - 1)
	{
		if (g_firstEntry + g_rowCount < g_entryCount)
		{
			g_firstEntry++;
		}
		else
		{
			g_firstEntry = 0;
			UIExternScrollMenuIterator_ActivateItem(menu, 0);
		}
	}
	else if (direction < 0 && row == 0)
	{
		if (g_firstEntry > 0)
		{
			g_firstEntry--;
		}
		else
		{
			g_firstEntry = g_entryCount - g_rowCount;
			UIExternScrollMenuIterator_ActivateItem(menu, g_rowCount - 1);
		}
	}
	else
	{
		return false;
	}

	UpdateRows(locker);
	UpdateScrollPercent();
	AptCallFunction("MPScroll", nullptr, "FE08", 0);
	SendSoundMsg(g_Addresses.iMsgPlaySoundMenuNavigatePtr);
	return true;
}

static void* __fastcall LoadAPX_AllocateBufferAndSeparateTextures_Hook(uintptr_t thisp)
{
	auto& stream = *reinterpret_cast<const uint8_t**>(thisp + 0x0);
	auto& streamLength = *reinterpret_cast<uint32_t*>(thisp + 0xC);

	if (IsLockerMovie(stream, streamLength))
	{
		g_isLockerMovieLoaded = true;
		g_isLockerMovieReplaced = IsVanillaLockerMovie(stream, streamLength);

		if (g_isLockerMovieReplaced)
		{
			g_lockerMovie.assign(g_FE08LockerMovie, g_FE08LockerMovie + sizeof(g_FE08LockerMovie));
			stream = g_lockerMovie.data();
			streamLength = static_cast<uint32_t>(g_lockerMovie.size());
		}
		else if (g_locker != 0)
		{
			*reinterpret_cast<int*>(g_locker + 0x1A8) = 2;
			*reinterpret_cast<int*>(g_locker + 0x220) = 2;
		}
	}

	return LoadAPX_AllocateBufferAndSeparateTextures.unsafe_thiscall<void*>(thisp);
}

static uintptr_t __fastcall FE08Locker_Constructor_Hook(uintptr_t thisp)
{
	FE08Locker_Constructor.unsafe_thiscall<uintptr_t>(thisp);

	g_locker = thisp;
	g_scrollBar = 0;
	g_scrollPercent = 0;
	g_page = PAGE_LOCKER;
	g_menuTitle = MenuPages[PAGE_LOCKER].title;
	g_entryCount = LOCKER_ROWS;
	g_rowCount = LOCKER_ROWS;
	g_firstEntry = 0;

	if (!g_isLockerMovieLoaded || g_isLockerMovieReplaced)
	{
		*reinterpret_cast<int*>(thisp + 0x1A8) = MAX_ROWS;
		*reinterpret_cast<int*>(thisp + 0x220) = LOCKER_ROWS;
	}

	return thisp;
}

static void __fastcall FE08Locker_Init_Hook(uintptr_t thisp)
{
	FE08Locker_Init.unsafe_thiscall<void>(thisp);

	uintptr_t subscriberVariable = thisp + 0x10;
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpTitle", &g_menuTitle, eValueType_char_readonly, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpVal", &g_rowValue, eValueType_char_readonly, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpValWidth", &g_rowValueWidth, eValueType_int32, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpRows", &g_rowCount, eValueType_int32, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpFirst", &g_firstEntry, eValueType_int32, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpCount", &g_entryCount, eValueType_int32, nullptr, nullptr, false);
	AptSubscriberVariable_AddVariable(subscriberVariable, "mpPercent", &g_scrollPercent, eValueType_int32, nullptr, nullptr, false);
}

static eAptHandleInput __fastcall FE08Locker_HandleInput_Hook(uintptr_t thisp, int, int cID)
{
	if (IsMenuActive())
	{
		int row = UIExternScrollMenuIterator_ActiveItemIndex(thisp + 0x5C);

		if (cID == INPUT_LEFT || cID == INPUT_RIGHT)
		{
			MenuOption* option = GetEntryOption(g_firstEntry + row);

			if (row >= 0 && option)
			{
				option->value = GetNextOptionValue(*option, cID == INPUT_RIGHT ? 1 : -1, option->type != OptionType::Decimal);
				UIExternScrollMenuIterator_UpdateItem(thisp + 0x5C, row, true);
				SendSoundMsg(g_Addresses.iMsgPlaySoundMenuNavigatePtr);
				return eAptHandleInput_Handled;
			}
		}
		else if ((cID == INPUT_UP || cID == INPUT_DOWN) && row >= 0 && ScrollPage(thisp, row, cID == INPUT_DOWN ? 1 : -1))
		{
			return eAptHandleInput_Handled;
		}
		else if (cID == INPUT_DEFAULTS && IsOptionPageActive())
		{
			ResetPage(thisp);
			return eAptHandleInput_Handled;
		}
		else if (cID == INPUT_SELECT && IsOptionPageActive())
		{
			AcceptPage(thisp);
			return eAptHandleInput_Handled;
		}
	}

	return FE08Locker_HandleInput.unsafe_thiscall<eAptHandleInput>(thisp, cID);
}

static bool __fastcall FE08Locker_OnInput_Select_Hook(uintptr_t thisp, int, int index)
{
	if (!g_isLockerMovieReplaced)
	{
		return FE08Locker_OnInput_Select.unsafe_thiscall<bool>(thisp, index);
	}

	if (g_page == PAGE_LOCKER)
	{
		if (index != MARKERPATCH_ROW)
		{
			return FE08Locker_OnInput_Select.unsafe_thiscall<bool>(thisp, index);
		}

		ShowPage(thisp, PAGE_MARKERPATCH, 0);
		return true;
	}

	int entry = g_firstEntry + index;

	if (g_page == PAGE_MARKERPATCH)
	{
		if (entry == LAA_PATCH_ENTRY)
		{
			SelectLAAPatch();
		}
		else
		{
			ShowPage(thisp, static_cast<MenuPage>(FIRST_OPTION_PAGE + entry), 0);
		}
	}

	return true;
}

static bool __fastcall FE08Locker_OnInput_Cancel_Hook(uintptr_t thisp, int)
{
	uint32_t flags = *reinterpret_cast<uint32_t*>(thisp + 0x23C);

	if (!IsMenuActive() || (flags & 4) != 0)
	{
		return FE08Locker_OnInput_Cancel.unsafe_thiscall<bool>(thisp);
	}

	if (g_page != PAGE_MARKERPATCH)
	{
		if (AreOptionsChanged(g_page))
		{
			ShowPopup("$UIPopup_SettingsChanged", "$UIPopup_DiscardSettings", true, OnDiscardPopupResult);
		}
		else
		{
			CancelPage(thisp);
		}

		return true;
	}

	ShowPage(thisp, PAGE_LOCKER, MARKERPATCH_ROW);

	if (g_isRestartPending)
	{
		g_isRestartPending = false;

		if (IsRestartNeeded())
		{
			ShowPopup("restart required", "Some of the changed settings apply after restarting the game. Restart now?", true, OnRestartPopupResult);
		}
	}

	return true;
}

static FE08MenuItem* __fastcall FE08Locker_DoUpdateItemAndLabel_Hook(uintptr_t thisp, int, FE08MenuItem* dst, const void* src, int index, const char* inLabel)
{
	g_rowValue = "";
	g_rowValueWidth = 0;

	if (!g_isLockerMovieReplaced || (g_page == PAGE_LOCKER && index != MARKERPATCH_ROW))
	{
		return FE08Locker_DoUpdateItemAndLabel.unsafe_thiscall<FE08MenuItem*>(thisp, dst, src, index, inLabel);
	}

	dst->mLabel = inLabel;

	if (g_page == PAGE_LOCKER)
	{
		dst->mName = "markerpatch";
		return dst;
	}

	int entry = g_firstEntry + index;
	dst->mName = GetEntryName(entry);
	g_rowValue = GetEntryValue(entry);
	g_rowValueWidth = GetTextWidth(g_rowValue);
	return dst;
}

static void __fastcall UIScrollBar_ScrollBy_Hook(uintptr_t thisp, int, int delta)
{
	if (thisp != g_scrollBar || !IsMenuActive())
	{
		UIScrollBar_ScrollBy.unsafe_thiscall<void>(thisp, delta);
		return;
	}

	ScrollTo(g_locker, g_firstEntry + delta);
	UpdateScrollPercent();
	AptCallFunction("MPScroll", nullptr, "FE08", 0);
}

static int __fastcall UIMenuScreenBase_OnHotSpotPressed_Hook(uintptr_t thisp, int, uint32_t name, int type, uint32_t flags)
{
	if (thisp == g_locker && IsOptionPageActive())
	{
		if (type == HOT_SPOT_MENU_ITEM) return eAptHandleInput_Handled;

		if (type == HOT_SPOT_HELP_BUTTON && (name == HELP_BUTTON_DEFAULTS || name == HELP_BUTTON_ACCEPT))
		{
			bool isInputDisabled = (*reinterpret_cast<uint32_t*>(thisp + 0x23C) & 4) != 0;

			if (!isInputDisabled)
			{
				name == HELP_BUTTON_ACCEPT ? AcceptPage(thisp) : ResetPage(thisp);
			}

			return eAptHandleInput_Handled;
		}
	}

	return UIMenuScreenBase_OnHotSpotPressed.unsafe_thiscall<int>(thisp, name, type, flags);
}

static void __fastcall UIHotSpotScreen_AddHelpButtons_Hook(uintptr_t thisp)
{
	UIHotSpotScreen_AddHelpButtons.unsafe_thiscall<void>(thisp);

	if (g_locker != 0 && thisp == g_locker + 0x160 && IsOptionPageActive())
	{
		UIHotSpotScreen_AddHelpButton(thisp, HELP_BUTTON_DEFAULTS, 0x5EEDEE23, INPUT_DEFAULTS);
		UIHotSpotScreen_AddHelpButton(thisp, HELP_BUTTON_ACCEPT, 0x126B8134, INPUT_SELECT);
	}
}

static void ApplyMarkerPatchMenu()
{
	if (!MarkerPatchMenu) return;

	for (int page = FIRST_OPTION_PAGE; page < FIRST_OPTION_PAGE + OPTION_PAGE_COUNT; page++)
	{
		for (int i = 0; i < MenuPages[page].optionCount; i++)
		{
			MenuOption& option = MenuPages[page].options[i];
			option.launchValue = GetOptionSetting(option);
			option.savedValue = option.launchValue;
			option.value = option.launchValue;
		}
	}

	g_Addresses.GPOPManagerPtr = GetAddress(Addr::GPOPManagerPtr);
	g_Addresses.iMsgPlaySoundMenuNavigatePtr = GetAddress(Addr::iMsgPlaySoundMenuNavigatePtr);
	g_Addresses.iMsgPlaySoundMenuSelectPtr = GetAddress(Addr::iMsgPlaySoundMenuSelectPtr);

	AptSubscriberVariable_AddVariable = reinterpret_cast<decltype(AptSubscriberVariable_AddVariable)>(GetAddress(Addr::AptSubscriberVariable_AddVariable));
	AptCallFunction = reinterpret_cast<decltype(AptCallFunction)>(GetAddress(Addr::AptCallFunction));
	UIExternScrollMenuIterator_ActiveItemIndex = reinterpret_cast<decltype(UIExternScrollMenuIterator_ActiveItemIndex)>(GetAddress(Addr::UIExternScrollMenuIterator_ActiveItemIndex));
	UIExternScrollMenuIterator_ActivateItem = reinterpret_cast<decltype(UIExternScrollMenuIterator_ActivateItem)>(GetAddress(Addr::UIExternScrollMenuIterator_ActivateItem));
	UIExternScrollMenuIterator_UpdateItem = reinterpret_cast<decltype(UIExternScrollMenuIterator_UpdateItem)>(GetAddress(Addr::UIExternScrollMenuIterator_UpdateItem));
	UIMenuScreenBase_GetScrollBar = reinterpret_cast<decltype(UIMenuScreenBase_GetScrollBar)>(GetAddress(Addr::UIMenuScreenBase_GetScrollBar));
	UIHotSpotScreen_AddHelpButton = reinterpret_cast<decltype(UIHotSpotScreen_AddHelpButton)>(GetAddress(Addr::UIHotSpotScreen_AddHelpButton));
	UIHotSpotScreen_UpdateHelpButtons = reinterpret_cast<decltype(UIHotSpotScreen_UpdateHelpButtons)>(GetAddress(Addr::UIHotSpotScreen_UpdateHelpButtons));
	GPOPManager_ShowPopup = reinterpret_cast<decltype(GPOPManager_ShowPopup)>(GetAddress(Addr::GPOPManager_ShowPopup));
	RWS__SendMsg = reinterpret_cast<decltype(RWS__SendMsg)>(GetAddress(Addr::RWS__SendMsg));

	LoadAPX_AllocateBufferAndSeparateTextures = HookHelper::CreateHook((void*)GetAddress(Addr::LoadAPX_AllocateBufferAndSeparateTextures), &LoadAPX_AllocateBufferAndSeparateTextures_Hook);
	FE08Locker_Constructor = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_Constructor), &FE08Locker_Constructor_Hook);
	FE08Locker_Init = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_Init), &FE08Locker_Init_Hook);
	FE08Locker_HandleInput = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_HandleInput), &FE08Locker_HandleInput_Hook);
	FE08Locker_OnInput_Select = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_OnInput_Select), &FE08Locker_OnInput_Select_Hook);
	FE08Locker_OnInput_Cancel = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_OnInput_Cancel), &FE08Locker_OnInput_Cancel_Hook);
	FE08Locker_DoUpdateItemAndLabel = HookHelper::CreateHook((void*)GetAddress(Addr::FE08Locker_DoUpdateItemAndLabel), &FE08Locker_DoUpdateItemAndLabel_Hook);
	UIScrollBar_ScrollBy = HookHelper::CreateHook((void*)GetAddress(Addr::UIScrollBar_ScrollBy), &UIScrollBar_ScrollBy_Hook);
	UIMenuScreenBase_OnHotSpotPressed = HookHelper::CreateHook((void*)GetAddress(Addr::UIMenuScreenBase_OnHotSpotPressed), &UIMenuScreenBase_OnHotSpotPressed_Hook);
	UIHotSpotScreen_AddHelpButtons = HookHelper::CreateHook((void*)GetAddress(Addr::UIHotSpotScreen_AddHelpButtons), &UIHotSpotScreen_AddHelpButtons_Hook);
}