#pragma once

#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES
#define NOMINMAX

#include <Windows.h>
#include <d3d9.h>

#include <stacktrace>
#include "ini.hpp"
#include "Controller.hpp"
#include "LAAPatcher.hpp"

#include "dllmain.hpp"
#include "helper.cpp"

#include "AchievementOverlay.hpp"

#pragma comment(lib, "SDL3-static.lib")


struct GlobalState
{
	// System initialization
	bool isInit = false;
	HMODULE GameModule = NULL;

	// Display configuration
	int screenWidth = 0;
	int screenHeight = 0;

	// Input configuration
	float mouseSens = 0.0f;
	bool isControllerActive = false;
	bool isXInverted = false;
	bool isYInverted = false;

	// Raw input state
	std::atomic<LONG> rawMouseDeltaX{ 0 };
	std::atomic<LONG> rawMouseDeltaY{ 0 };
	LONG frameRawX = 0;
	LONG frameRawY = 0;

	// Physics
	float frameTimeScale = 0.0f;
	float constraintMass = 0.0f;
	int deathFrameCount = 0;

	// Scripted aiming scene context
	int* momentumAimOuter = nullptr;
	uintptr_t momentumAimData = 0;
	int* boundedAimOuter = nullptr;
	uintptr_t coneAimData = 0;
	int* oscillatingAimOuter = nullptr;

	// Flare fix
	IDirect3DDevice9* device = nullptr;
	bool inFlareDraw = false;
	IDirect3DBaseTexture9* sourceTex = nullptr;
	IDirect3DTexture9* proxyTex = nullptr;
	IDirect3DSurface9* proxySurf = nullptr;
	bool resourcesValid = false;
	bool snapshotValid = false;

	// Misc
	bool isLoadingShopItems = false;
	bool forceCurrentItem = false;
	int16_t currentHeight = 0;
	float resolutionScale = 0.0f;
	float frameTime = 0.0f;
	bool elevatorFixArmed = false;
};

// Global instance
GlobalState g_State;

struct GameAddresses
{
	DWORD DevicePtr = 0;
	DWORD InputManagerPtr = 0;
	DWORD NgGamePlusPtr = 0;
	DWORD LoadedSaveMemoryPtr = 0;
	DWORD TargetFrameTimeMsPtr = 0;
	DWORD EngineFrameTimePtr = 0;
	DWORD UpsideDownYawMin = 0;
	DWORD UpsideDownYawMax = 0;
	DWORD UpsideDownPitchMin = 0;
	DWORD UpsideDownPitchMax = 0;
	DWORD CameraRollToYawCoef = 0;
};

// Memory addresses
GameAddresses g_Addresses;

static constexpr float TARGET_FRAME_TIME = 1.0f / 30.0f;
static constexpr float PITCH_LIMIT_NORMAL = M_PI / 3.0f;
static constexpr float PITCH_LIMIT_AIM_DOWN = -5.0f * M_PI / 12.0f;
static constexpr float DEG2RAD = 0.017453292f;

// =============================
// Ini Variables
// =============================

// Fixes
bool HavokPhysicsFix = false;
bool HighCoreCPUFix = false;
bool VSyncRefreshRateFix = false;
bool FixDifficultyRewards = false;
bool FixSuitIDConflicts = false;
bool FixSaveStringHandling = false;
bool FixAutomaticWeaponFireRate = false;
bool FixSolarArrayElevator = false;
bool FixBlurResolution = false;
bool FixShadowBlur = false;
bool FixFlareArtifacts = false;
bool FixGlassReflections = false;

// General
bool AchievementSupport = false;
bool DisableOnlineFeatures = false;
bool IncreasedEntityPersistence = false;
int IncreasedEntityPersistenceBodies = 0;
int IncreasedEntityPersistenceLimbs = 0;
bool IncreasedDecalPersistence = false;
bool SkipIntro = false;
bool SkipArtificialLoadingDelay = false;
int CheckLAAPatch = 0;

// Display
bool AutoResolution = false;
bool FontScaling = false;
float FontScalingFactor = 0;

// Input
bool RawMouseInput = false;
bool UseSDLControllerInput = false;
bool BlockDirectInputDevices = false;
bool DisableKeyboardHook = false;
bool GyroEnabled = false;
float GyroSensitivity = 0.0f;
float GyroSmoothing = 0.0f;
bool GyroCalibrationPersistence = false;
bool TouchpadEnabled = false;
bool InvertABXYButtons = false;

// Graphics
int MaxAnisotropy = 0;
bool ForceTrilinearFiltering = false;
int DynamicShadowResolution = 0;

// DLC
bool EnableHazardPack = false;
bool EnableMartialLawPack = false;
bool EnableSupernovaPack = false;
bool EnableSeveredDLC = false;
bool EnableHackerDLC = false;
bool EnableZealotDLC = false;
bool EnableRivetGunDLC = false;

static DWORD ScanModuleSignature(HMODULE Module, std::string_view Signature, const char* PatchName, int FunctionStartCheckCount = -1, bool ShowError = true)
{
	DWORD Address = MemoryHelper::FindSignatureAddress(Module, Signature, FunctionStartCheckCount);

	if (Address == 0 && ShowError)
	{
		std::string ErrorMessage = "Error: Unable to find signature for patch: ";
		ErrorMessage += PatchName;
		MessageBoxA(NULL, ErrorMessage.c_str(), "MarkerPatch", MB_ICONERROR);
	}

	return Address;
}

static float CalculateFpsConstant(int target_fps)
{
	// Disable limiter for unlimited FPS
	if (target_fps <= 0)
	{
		return 0.0f;
	}

	float frame_time_ms = 1000.0f / (float)target_fps;
	return frame_time_ms / 2.0f; // Divide by 2 since a1 = 2
}

static inline bool MatchId(const DWORD* item, DWORD d0, DWORD d1, DWORD d2, DWORD d3)
{
	return item[0] == d0 && item[1] == d1 && item[2] == d2 && item[3] == d3;
}

static IDirect3DDevice9* GetD3D9Device()
{
	if (g_State.device) return g_State.device;
	IDirect3DDevice9* dev = nullptr;

	__try
	{
		dev = *reinterpret_cast<IDirect3DDevice9**>(g_Addresses.DevicePtr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return nullptr;
	}

	g_State.device = dev;
	return dev;
}

static bool IsUALPresent()
{
	for (const auto& entry : std::stacktrace::current())
	{
		HMODULE hModule = NULL;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)entry.native_handle(), &hModule))
		{
			if (GetProcAddress(hModule, "IsUltimateASILoader") != NULL)
				return true;
		}
	}

	return false;
}
