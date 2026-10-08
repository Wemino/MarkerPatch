#pragma once

#include "../Globals.cpp"

static void ReadConfig()
{
	IniHelper::Init();

	// Fixes
	HavokPhysicsFix = IniHelper::ReadInteger("Fixes", "HavokPhysicsFix", 1) == 1;
	HighCoreCPUFix = IniHelper::ReadInteger("Fixes", "HighCoreCPUFix", 1) == 1;
	ThreadAffinityFix = IniHelper::ReadInteger("Fixes", "ThreadAffinityFix", 1) == 1;
	VSyncRefreshRateFix = IniHelper::ReadInteger("Fixes", "VSyncRefreshRateFix", 1) == 1;
	FixFrameLimiter = IniHelper::ReadInteger("Fixes", "FixFrameLimiter", 1) == 1;
	FixDifficultyRewards = IniHelper::ReadInteger("Fixes", "FixDifficultyRewards", 1) == 1;
	FixSuitIDConflicts = IniHelper::ReadInteger("Fixes", "FixSuitIDConflicts", 1) == 1;
	FixSaveStringHandling = IniHelper::ReadInteger("Fixes", "FixSaveStringHandling", 1) == 1;
	FixAutomaticWeaponFireRate = IniHelper::ReadInteger("Fixes", "FixAutomaticWeaponFireRate", 1) == 1;
	FixSolarArrayElevator = IniHelper::ReadInteger("Fixes", "FixSolarArrayElevator", 1) == 1;
	FixBlurResolution = IniHelper::ReadInteger("Fixes", "FixBlurResolution", 1) == 1;
	FixShadowBlur = IniHelper::ReadInteger("Fixes", "FixShadowBlur", 1) == 1;
	FixFlareArtifacts = IniHelper::ReadInteger("Fixes", "FixFlareArtifacts", 1) == 1;
	FixVertexNormals = IniHelper::ReadInteger("Fixes", "FixVertexNormals", 1) == 1;
	FixClothPhysics = IniHelper::ReadInteger("Fixes", "FixClothPhysics", 1) == 1;
	FixMenuSpeed = IniHelper::ReadInteger("Fixes", "FixMenuSpeed", 1) == 1;
	FixGameClock = IniHelper::ReadInteger("Fixes", "FixGameClock", 1) == 1;
	FixMainLoopSpin = IniHelper::ReadInteger("Fixes", "FixMainLoopSpin", 1) == 1;
	FixStreamingBudget = IniHelper::ReadInteger("Fixes", "FixStreamingBudget", 1) == 1;
	FixAudioSyncStall = IniHelper::ReadInteger("Fixes", "FixAudioSyncStall", 1) == 1;
	PreloadStreamedTextures = IniHelper::ReadInteger("Fixes", "PreloadStreamedTextures", 1) == 1;
	FixInputHistory = IniHelper::ReadInteger("Fixes", "FixInputHistory", 1) == 1;
	FixImpalingProjectiles = IniHelper::ReadInteger("Fixes", "FixImpalingProjectiles", 1) == 1;
	FixExplosionDamage = IniHelper::ReadInteger("Fixes", "FixExplosionDamage", 1) == 1;
	FixOffscreenEffects = IniHelper::ReadInteger("Fixes", "FixOffscreenEffects", 1) == 1;

	// General
	AchievementSupport = IniHelper::ReadInteger("General", "AchievementSupport", 1) == 1;
	AchievementOverlay::g_toggleKey = IniHelper::ReadInteger("General", "AchievementOverlayKey", VK_HOME);
	DisableOnlineFeatures = IniHelper::ReadInteger("General", "DisableOnlineFeatures", 1) == 1;
	IncreasedEntityPersistence = IniHelper::ReadInteger("General", "IncreasedEntityPersistence", 1) == 1;
	IncreasedEntityPersistenceBodies = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceBodies", 25);
	IncreasedEntityPersistenceLimbs = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceLimbs", 96);
	IncreasedDecalPersistence = IniHelper::ReadInteger("General", "IncreasedDecalPersistence", 1) == 1;
	SkipIntro = IniHelper::ReadInteger("General", "SkipIntro", 0) == 1;
	SkipArtificialLoadingDelay = IniHelper::ReadInteger("General", "SkipArtificialLoadingDelay", 1) == 1;
	LoadASIPlugins = IniHelper::ReadInteger("General", "LoadASIPlugins", 1) == 1;
	CheckLAAPatch = IniHelper::ReadInteger("General", "CheckLAAPatch", 0);

	// Display
	AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1) == 1;
	FontScaling = IniHelper::ReadInteger("Display", "FontScaling", 1) == 1;
	FontScalingFactor = IniHelper::ReadFloat("Display", "FontScalingFactor", 1.0f);
	FOVScale = IniHelper::ReadFloat("Display", "FOVScale", 1.0f);

	// Input
	RawMouseInput = IniHelper::ReadInteger("Input", "RawMouseInput", 1) == 1;
	UseSDLControllerInput = IniHelper::ReadInteger("Input", "UseSDLControllerInput", 1) == 1;
	BlockDirectInputDevices = IniHelper::ReadInteger("Input", "BlockDirectInputDevices", 1) == 1;
	DisableKeyboardHook = IniHelper::ReadInteger("Input", "DisableKeyboardHook", 1) == 1;
	ExtraMouseButtonBinding = IniHelper::ReadInteger("Input", "ExtraMouseButtonBinding", 1) == 1;
	AutoHideMouseCursor = IniHelper::ReadInteger("Input", "AutoHideMouseCursor", 1) == 1;
	GyroEnabled = IniHelper::ReadInteger("Input", "GyroEnabled", 0) == 1;
	GyroSensitivity = IniHelper::ReadFloat("Input", "GyroSensitivity", 1.0f);
	GyroSmoothing = IniHelper::ReadFloat("Input", "GyroSmoothing", 0.016f);
	GyroCalibrationPersistence = IniHelper::ReadInteger("Input", "GyroCalibrationPersistence", 1) == 1;
	TouchpadEnabled = IniHelper::ReadInteger("Input", "TouchpadEnabled", 1) == 1;
	InvertABXYButtons = IniHelper::ReadInteger("Input", "InvertABXYButtons", 1) == 1;

	// Graphics
	MaxAnisotropy = IniHelper::ReadInteger("Graphics", "MaxAnisotropy", 16);
	DynamicShadowResolution = IniHelper::ReadInteger("Graphics", "DynamicShadowResolution", 1920);
	ImprovedAntiAliasingMode = IniHelper::ReadInteger("Graphics", "ImprovedAntiAliasingMode", 2);
	SSAAScale = IniHelper::ReadFloat("Graphics", "SSAAScale", 2.0f);

	// Modding
	DumpArchiveAssets = IniHelper::ReadInteger("Modding", "DumpArchiveAssets", 0) == 1;
	LoadModFiles = IniHelper::ReadInteger("Modding", "LoadModFiles", 1) == 1;

	// DLC
	EnableHazardPack = IniHelper::ReadInteger("DLC", "EnableHazardPack", 0) == 1;
	EnableMartialLawPack = IniHelper::ReadInteger("DLC", "EnableMartialLawPack", 0) == 1;
	EnableSupernovaPack = IniHelper::ReadInteger("DLC", "EnableSupernovaPack", 0) == 1;
	EnableSeveredDLC = IniHelper::ReadInteger("DLC", "EnableSeveredDLC", 0) == 1;
	EnableHackerDLC = IniHelper::ReadInteger("DLC", "EnableHackerDLC", 0) == 1;
	EnableZealotDLC = IniHelper::ReadInteger("DLC", "EnableZealotDLC", 0) == 1;
	EnableRivetGunDLC = IniHelper::ReadInteger("DLC", "EnableRivetGunDLC", 0) == 1;
	EnableIgnitionRooms = IniHelper::ReadInteger("DLC", "EnableIgnitionRooms", 0) == 1;
	EnableOriginalPlasmaCutter = IniHelper::ReadInteger("DLC", "EnableOriginalPlasmaCutter", 0) == 1;

	if (AutoResolution || UseSDLControllerInput)
	{
		auto [screenWidth, screenHeight] = SystemHelper::GetScreenResolution();
		g_State.screenWidth = screenWidth;
		g_State.screenHeight = screenHeight;

		ControllerHelper::SetTouchpadDimensions(screenWidth, screenHeight);
	}

	MaxAnisotropy = std::clamp(MaxAnisotropy, 0, 16);
	FOVScale = std::clamp(FOVScale, 0.5f, 2.0f);
	ImprovedAntiAliasingMode = std::clamp(ImprovedAntiAliasingMode, 0, 3);
	SSAAScale = std::clamp(SSAAScale, 1.0f, 4.0f);

	// Set a maximum so that the game doesn't crash
	IncreasedEntityPersistenceBodies = std::clamp(IncreasedEntityPersistenceBodies, 0, 35);
	IncreasedEntityPersistenceLimbs = std::clamp(IncreasedEntityPersistenceLimbs, 0, 120);

	// Init SDL variables
	ControllerHelper::SetGyroEnabled(GyroEnabled);
	ControllerHelper::SetGyroSensitivity(GyroSensitivity);
	ControllerHelper::SetGyroSmoothing(GyroSmoothing);
	ControllerHelper::SetTouchpadEnabled(TouchpadEnabled);
	ControllerHelper::SetGyroCalibrationPersistence(GyroCalibrationPersistence);
}

static void Init()
{
	ReadConfig();

	if (CheckLAAPatch)
	{
		LAAPatcher::PerformLAAPatch(GetModuleHandleA(NULL), CheckLAAPatch != 2);
	}

	// Fixes
	ApplyHavokPhysicsFix();
	ApplyHighCoreCPUFix();
	ApplyThreadAffinityFix();
	ApplyVSyncRefreshRateFix();
	ApplyFixFrameLimiter();
	ApplyFixDifficultyRewards();
	ApplyFixSuitIDConflicts();
	ApplyFixSaveStringHandling();
	ApplyFixAutomaticWeaponFireRate();
	ApplyFixSolarArrayElevator();
	ApplyFixBlurResolution();
	ApplyFixShadowBlur();
	ApplyFixFlareArtifacts();
	ApplyFixVertexNormals();
	ApplyFixClothPhysics();
	ApplyFixMenuSpeed();
	ApplyFixGameClock();
	ApplyFixMainLoopSpin();
	ApplyFixStreamingBudget();
	ApplyFixAudioSyncStall();
	ApplyFixInputHistory();
	ApplyFixImpalingProjectiles();
	ApplyFixExplosionDamage();
	ApplyFixOffscreenEffects();

	// General
	ApplyAchievementSupport();
	ApplyDisableOnlineFeatures();
	ApplyIncreasedEntityPersistence();
	ApplyIncreasedDecalPersistence();
	ApplySkipIntro();
	ApplySkipArtificialLoadingDelay();

	// Display
	ApplyAutoResolution();
	ApplyFontScaling();
	ApplyFOVScaling();

	// Input
	ApplyRawMouseInput();
	ApplyFilterInputDevices();
	ApplyDisableKeyboardHook();
	ApplyExtraMouseButtonBinding();
	ApplyAutoHideMouseCursor();
	ApplyUseSDLControllerInput();

	// Graphics
	ApplyTextureFiltering();
	ApplyDynamicShadowResolution();
	ApplyImprovedAntiAliasing();

	// Modding
	ApplyArchiveDump();
	ApplyModFiles();
	ApplyArchiveStreamHook();

	// DLC
	ApplyBonusUnlocks();

	// Misc
	ApplyMainLoopHook();
	ApplyResolutionHook();
	ApplyD3D9ApiMidHook();
	ApplyASILoader();
}

static bool IsBuildDate(uintptr_t base, uintptr_t address, const char* date)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
	IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

	uintptr_t offset = address - 0x400000;
	size_t length = strlen(date) + 1;
	if (offset + length > nt->OptionalHeader.SizeOfImage) return false;

	return memcmp(reinterpret_cast<const void*>(base + offset), date, length) == 0;
}

safetyhook::InlineHook regOpenKeyHook;
static LSTATUS WINAPI RegOpenKeyExW_Hook(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	// If the execution of the game started
	if (!g_State.isInit && samDesired == 0x20019 && lpSubKey && wcscmp(lpSubKey, L"SOFTWARE\\EA Games\\Dead Space 2") == 0)
	{
		g_State.isInit = true; // Make sure to never enter this condition again
		g_State.GameModule = GetModuleHandleA(NULL);
		(void)regOpenKeyHook.disable();

		// The DRM strips the timestamp, so check the __DATE__ string that each build keeps in .rdata
		uintptr_t base = (uintptr_t)g_State.GameModule;
		GameBuild build = GameBuild::Unknown;

		if (IsBuildDate(base, 0x1B92078, "Feb 24 2011")) // Current Steam/EA App version
		{
			build = GameBuild::Current;
		}
		else if (IsBuildDate(base, 0x1B91040, "Dec 14 2010"))
		{
			build = GameBuild::V1_0;
		}
		else
		{
			MessageBoxA(NULL, "This .exe is not supported.", "MarkerPatch", MB_ICONERROR);
			return regOpenKeyHook.stdcall<LSTATUS>(hKey, lpSubKey, ulOptions, samDesired, phkResult);
		}

		Addresses::SetBuild(build, base);
		Init();
	}

	return regOpenKeyHook.stdcall<LSTATUS>(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}
