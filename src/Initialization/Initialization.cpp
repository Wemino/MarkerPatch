#pragma once

#include "../Globals.cpp"

static void ReadConfig()
{
	IniHelper::Init();

	// Fixes
	HavokPhysicsFix = IniHelper::ReadInteger("Fixes", "HavokPhysicsFix", 1) == 1;
	HighCoreCPUFix = IniHelper::ReadInteger("Fixes", "HighCoreCPUFix", 1) == 1;
	VSyncRefreshRateFix = IniHelper::ReadInteger("Fixes", "VSyncRefreshRateFix", 1) == 1;
	FixDifficultyRewards = IniHelper::ReadInteger("Fixes", "FixDifficultyRewards", 1) == 1;
	FixSuitIDConflicts = IniHelper::ReadInteger("Fixes", "FixSuitIDConflicts", 1) == 1;
	FixSaveStringHandling = IniHelper::ReadInteger("Fixes", "FixSaveStringHandling", 1) == 1;
	FixAutomaticWeaponFireRate = IniHelper::ReadInteger("Fixes", "FixAutomaticWeaponFireRate", 1) == 1;
	FixSolarArrayElevator = IniHelper::ReadInteger("Fixes", "FixSolarArrayElevator", 1) == 1;
	FixBlurResolution = IniHelper::ReadInteger("Fixes", "FixBlurResolution", 1) == 1;
	FixShadowBlur = IniHelper::ReadInteger("Fixes", "FixShadowBlur", 1) == 1;
	FixFlareArtifacts = IniHelper::ReadInteger("Fixes", "FixFlareArtifacts", 1) == 1;
	FixGlassReflections = IniHelper::ReadInteger("Fixes", "FixGlassReflections", 1) == 1;

	// General
	AchievementSupport = IniHelper::ReadInteger("General", "AchievementSupport", 1) == 1;
	DisableOnlineFeatures = IniHelper::ReadInteger("General", "DisableOnlineFeatures", 1) == 1;
	IncreasedEntityPersistence = IniHelper::ReadInteger("General", "IncreasedEntityPersistence", 1) == 1;
	IncreasedEntityPersistenceBodies = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceBodies", 25);
	IncreasedEntityPersistenceLimbs = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceLimbs", 96);
	IncreasedDecalPersistence = IniHelper::ReadInteger("General", "IncreasedDecalPersistence", 1) == 1;
	SkipIntro = IniHelper::ReadInteger("General", "SkipIntro", 0) == 1;
	SkipArtificialLoadingDelay = IniHelper::ReadInteger("General", "SkipArtificialLoadingDelay", 1) == 1;
	CheckLAAPatch = IniHelper::ReadInteger("General", "CheckLAAPatch", 0);

	// Display
	AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1) == 1;
	FontScaling = IniHelper::ReadInteger("Display", "FontScaling", 1) == 1;
	FontScalingFactor = IniHelper::ReadFloat("Display", "FontScalingFactor", 1.0f);

	// Input
	RawMouseInput = IniHelper::ReadInteger("Input", "RawMouseInput", 1) == 1;
	UseSDLControllerInput = IniHelper::ReadInteger("Input", "UseSDLControllerInput", 1) == 1;
	BlockDirectInputDevices = IniHelper::ReadInteger("Input", "BlockDirectInputDevices", 1) == 1;
	GyroEnabled = IniHelper::ReadInteger("Input", "GyroEnabled", 0) == 1;
	GyroSensitivity = IniHelper::ReadFloat("Input", "GyroSensitivity", 1.0f);
	GyroSmoothing = IniHelper::ReadFloat("Input", "GyroSmoothing", 0.016f);
	GyroCalibrationPersistence = IniHelper::ReadInteger("Input", "GyroCalibrationPersistence", 1) == 1;
	TouchpadEnabled = IniHelper::ReadInteger("Input", "TouchpadEnabled", 1) == 1;
	InvertABXYButtons = IniHelper::ReadInteger("Input", "InvertABXYButtons", 1) == 1;

	// Graphics
	MaxAnisotropy = IniHelper::ReadInteger("Graphics", "MaxAnisotropy", 16);
	ForceTrilinearFiltering = IniHelper::ReadInteger("Graphics", "ForceTrilinearFiltering", 1) == 1;
	DynamicShadowResolution = IniHelper::ReadInteger("Graphics", "DynamicShadowResolution", 1920);

	// DLC
	EnableHazardPack = IniHelper::ReadInteger("DLC", "EnableHazardPack", 0) == 1;
	EnableMartialLawPack = IniHelper::ReadInteger("DLC", "EnableMartialLawPack", 0) == 1;
	EnableSupernovaPack = IniHelper::ReadInteger("DLC", "EnableSupernovaPack", 0) == 1;
	EnableSeveredDLC = IniHelper::ReadInteger("DLC", "EnableSeveredDLC", 0) == 1;
	EnableHackerDLC = IniHelper::ReadInteger("DLC", "EnableHackerDLC", 0) == 1;
	EnableZealotDLC = IniHelper::ReadInteger("DLC", "EnableZealotDLC", 0) == 1;
	EnableRivetGunDLC = IniHelper::ReadInteger("DLC", "EnableRivetGunDLC", 0) == 1;

	if (AutoResolution || UseSDLControllerInput)
	{
		auto [screenWidth, screenHeight] = SystemHelper::GetScreenResolution();
		g_State.screenWidth = screenWidth;
		g_State.screenHeight = screenHeight;

		ControllerHelper::SetTouchpadDimensions(screenWidth, screenHeight);
	}

	MaxAnisotropy = std::clamp(MaxAnisotropy, 0, 16);

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
	ApplyVSyncRefreshRateFix();
	ApplyFixDifficultyRewards();
	ApplyFixSuitIDConflicts();
	ApplyFixSaveStringHandling();
	ApplyFixAutomaticWeaponFireRate();
	ApplyFixSolarArrayElevator();
	ApplyFixBlurResolution();
	ApplyFixShadowBlur();
	ApplyFixFlareArtifacts();
	ApplyFixGlassReflections();

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

	// Input
	ApplyRawMouseInput();
	ApplyFilterInputDevices();
	ApplyUseSDLControllerInput();

	// Graphics
	ApplyTextureFiltering();
	ApplyDynamicShadowResolution();

	// DLC
	ApplyShopHooks();

	// Misc
	ApplyMainLoopHook();
	ApplyResolutionHook();
	ApplyD3D9ApiMidHook();
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
		Init();
	}

	return regOpenKeyHook.stdcall<LSTATUS>(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}
