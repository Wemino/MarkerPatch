#pragma once

#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES
#define NOMINMAX

#include <Windows.h>
#include <d3d9.h>

#include <atomic>
#include <deque>
#include <intrin.h>
#include <mutex>
#include <stacktrace>
#include <system_error>
#include <unordered_set>
#include "ini.hpp"
#include "Controller.hpp"
#include "LAAPatcher.hpp"

#include "dllmain.hpp"
#include "helper.cpp"
#include "Addresses.hpp"

#include "AchievementOverlay.hpp"

#pragma comment(lib, "SDL3-static.lib")

// Windows 10 1803 and later, missing from older SDKs
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif


struct GlobalState
{
	// System initialization
	bool isInit = false;
	HMODULE GameModule = NULL;

	bool isUALPresent = false;

	// Display configuration
	int screenWidth = 0;
	int screenHeight = 0;

	// Input configuration
	float mouseSens = 0.0f;
	bool isControllerActive = false;
	bool isXInverted = false;
	bool isYInverted = false;
	uintptr_t mouseDeviceState = 0;
	bool isMouseCursorVisible = false;

	// Raw input state
	std::atomic<LONG> rawMouseDeltaX{ 0 };
	std::atomic<LONG> rawMouseDeltaY{ 0 };
	LONG frameRawX = 0;
	LONG frameRawY = 0;
	float frameGyroYaw = 0.0f;
	float frameGyroPitch = 0.0f;
	uintptr_t mouseAimData = 0;
	float mouseAimPitch = 0.0f;
	float mouseAimYaw = 0.0f;

	// Flare fix
	IDirect3DDevice9* device = nullptr;
	bool inFlareDraw = false;
	IDirect3DBaseTexture9* sourceTex = nullptr;
	IDirect3DTexture9* proxyTex = nullptr;
	IDirect3DSurface9* proxySurf = nullptr;
	bool resourcesValid = false;
	bool snapshotValid = false;

	// Fire rate fix
	DWORD lastShotTime = 0;
	DWORD readyTime = 0;
	float shotError = 0.0f;
	bool usesFireAnim = true;

	// Frame timing
	LARGE_INTEGER qpcFrequency{};
	LONGLONG nextFrameCounter = 0;
	HANDLE frameLimiterTimer = nullptr;
	double frameLimiterSpinMs = 1.0;

	// Main loop spin fix
	HANDLE mainLoopTimer = nullptr;
	int framesBeforeMix = 0;

	// Streaming fix
	double refreshPeriodMs = 1000.0 / 60.0;

	// Audio sync fix
	HANDLE dacWakeEvent = nullptr;
	std::atomic<int> extraMixChunks{ 0 };
	std::atomic<int> maxExtraMixChunks{ -1 };
	std::atomic<DWORD> lastCommandWaitTime{ 0 };

	// Menu speed fix
	double aptTime = 0.0;
	float menuStepTime = 0.0f;
	float menuFrameSteps = 1.0f;
	bool isMenuStep = true;

	// Misc
	bool isLoadingShopItems = false;
	bool forceCurrentItem = false;
	int16_t currentHeight = 0;
	float resolutionScale = 0.0f;
	float frameTime = 0.0f;
	bool elevatorFixArmed = false;
	bool isCopyingCamera = false;
	bool isSupersampling = false;
};

// Global instance
GlobalState g_State;

struct GameAddresses
{
	DWORD DevicePtr = 0;
	DWORD InputDeviceManagerPtr = 0;
	DWORD UIFrontendManagerPtr = 0;
	DWORD OptionsDifficultyPtr = 0;
	DWORD TargetFrameTimeMsPtr = 0;
	DWORD FrameTimeSecPtr = 0;
	DWORD SimTimeElapsedMSecPtr = 0;
	DWORD FrameLimiterEnabledPtr = 0;
	DWORD UnlockHandlerPtr = 0;
	DWORD AAValsFlagsPtr = 0;
	DWORD FrameCopyValidPtr = 0;
	DWORD RenderStatesPtr = 0;
	DWORD SamplerStatesPtr = 0;
	DWORD VertexShaderPtr = 0;
	DWORD PixelShaderPtr = 0;
	DWORD VertexDeclarationPtr = 0;
	DWORD StreamSourcesPtr = 0;
	DWORD VertexShaderConstantsPtr = 0;
	DWORD PixelShaderConstantsPtr = 0;
	DWORD PresentParamsPtr = 0;
	DWORD HangingMinYawPtr = 0;
	DWORD HangingMaxYawPtr = 0;
	DWORD HangingMinPitchPtr = 0;
	DWORD HangingMaxPitchPtr = 0;
	DWORD HangingYawFactorPtr = 0;
	DWORD ResponseCurvePtr = 0;
	DWORD PlayerSpeedSettingsPtr = 0;
	DWORD SoundProviderPtr = 0;
	DWORD PresentModePtr = 0;
	DWORD PresentIntervalsPtr = 0;
};

// Memory addresses
GameAddresses g_Addresses;

static constexpr float TARGET_FRAME_TIME = 1.0f / 30.0f;
static constexpr float PITCH_LIMIT_NORMAL = M_PI / 3.0f;
static constexpr float PITCH_LIMIT_AIM_DOWN = -5.0f * M_PI / 12.0f;
static constexpr float DEG2RAD = 0.017453292f;
static constexpr float MAX_BLUR_RADIUS = 20.0f;

enum AntiAliasingMode
{
	AA_DISABLED,
	AA_FXAA,
	AA_SMAA,
	AA_SSAA
};

// =============================
// Ini Variables
// =============================

// Fixes
bool HavokPhysicsFix = false;
bool HighCoreCPUFix = false;
bool ThreadAffinityFix = false;
bool VSyncRefreshRateFix = false;
bool FixFrameLimiter = false;
bool FixDifficultyRewards = false;
bool FixSuitIDConflicts = false;
bool FixSaveStringHandling = false;
bool FixAutomaticWeaponFireRate = false;
bool FixSolarArrayElevator = false;
bool FixBlurResolution = false;
bool FixShadowBlur = false;
bool FixFlareArtifacts = false;
bool FixVertexNormals = false;
bool FixClothPhysics = false;
bool FixMenuSpeed = false;
bool FixGameClock = false;
bool FixMainLoopSpin = false;
bool FixStreamingBudget = false;
bool FixAudioSyncStall = false;
bool PreloadStreamedTextures = false;
bool FixInputHistory = false;
bool FixImpalingProjectiles = false;
bool FixExplosionDamage = false;
bool FixOffscreenEffects = false;

// General
bool AchievementSupport = false;
bool DisableOnlineFeatures = false;
bool IncreasedEntityPersistence = false;
int IncreasedEntityPersistenceBodies = 0;
int IncreasedEntityPersistenceLimbs = 0;
bool IncreasedDecalPersistence = false;
bool SkipIntro = false;
bool SkipArtificialLoadingDelay = false;
bool LoadASIPlugins = false;
int CheckLAAPatch = 0;

// Display
bool AutoResolution = false;
bool FontScaling = false;
float FontScalingFactor = 0;
float FOVScale = 0.0f;

// Input
bool RawMouseInput = false;
bool UseSDLControllerInput = false;
bool BlockDirectInputDevices = false;
bool DisableKeyboardHook = false;
bool ExtraMouseButtonBinding = false;
bool AutoHideMouseCursor = false;
bool GyroEnabled = false;
float GyroSensitivity = 0.0f;
float GyroSmoothing = 0.0f;
bool GyroCalibrationPersistence = false;
bool TouchpadEnabled = false;
bool InvertABXYButtons = false;

// Graphics
int MaxAnisotropy = 0;
int DynamicShadowResolution = 0;
int ImprovedAntiAliasingMode = 0;
float SSAAScale = 0.0f;

// Modding
bool DumpArchiveAssets = false;
bool LoadModFiles = false;

// DLC
bool EnableHazardPack = false;
bool EnableMartialLawPack = false;
bool EnableSupernovaPack = false;
bool EnableSeveredDLC = false;
bool EnableHackerDLC = false;
bool EnableZealotDLC = false;
bool EnableRivetGunDLC = false;
bool EnableIgnitionRooms = false;
bool EnableOriginalPlasmaCutter = false;

static std::filesystem::path BuildLongPath(const std::string& root, const std::string& relativePath)
{
	return std::filesystem::path("\\\\?\\" + root + "\\" + relativePath);
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
