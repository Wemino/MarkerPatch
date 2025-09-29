#define MINI_CASE_SENSITIVE
#define _USE_MATH_DEFINES

#include <Windows.h>
#include <dinput.h>

#include <stacktrace>

#include "ini.hpp"
#include "dllmain.hpp"
#include "helper.hpp"

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

	// Camera state
	bool isInNormalCamera = false;
	int aimingPassCount = 0;

	// Physics
	float frameTimeScale = 0;
	float constraintMass = 0;
	int deathFrameCount = 0;

	// Misc
	bool isLoadingShopItems = false;
	float frameTime = 0;
};

// Global instance
GlobalState g_State;

struct GameAddresses
{
	DWORD ItemCheckAddress = 0;
	DWORD InputManagerPtr = 0;
	DWORD NgGamePlusPtr = 0;
	DWORD LoadedSaveMemoryPtr = 0;
	DWORD TargetFrameTimeMsPtr = 0;
	DWORD EngineFrameTimePtr = 0;
};

// Memory addresses
GameAddresses g_Addresses;

static constexpr float TARGET_FRAME_TIME = 1.0f / 30.0f;

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

// General
bool DisableOnlineFeatures = false;
bool IncreasedEntityPersistence = false;
int IncreasedEntityPersistenceBodies = 0;
int IncreasedEntityPersistenceLimbs = 0;
bool SkipIntro = false;

// Display
bool AutoResolution = false;
bool FontScaling = false;
float FontScalingFactor = 0;

// Input
bool RawMouseInput = false;
bool BlockDirectInputDevices = false;

// Graphics
int MaxAnisotropy = 0;
bool ForceTrilinearFiltering = false;

// DLC
bool EnableHazardPack = false;
bool EnableMartialLawPack = false;
bool EnableSupernovaPack = false;
bool EnableSeveredDLC = false;
bool EnableHackerDLC = false;
bool EnableZealotDLC = false;
bool EnableRivetGunDLC = false;

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

	// General
	DisableOnlineFeatures = IniHelper::ReadInteger("General", "DisableOnlineFeatures", 1) == 1;
	IncreasedEntityPersistence = IniHelper::ReadInteger("General", "IncreasedEntityPersistence", 1) == 1;
	IncreasedEntityPersistenceBodies = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceBodies", 25);
	IncreasedEntityPersistenceLimbs = IniHelper::ReadInteger("General", "IncreasedEntityPersistenceLimbs", 96);
	SkipIntro = IniHelper::ReadInteger("General", "SkipIntro", 0) == 1;

	// Display
	AutoResolution = IniHelper::ReadInteger("Display", "AutoResolution", 1) == 1;
	FontScaling = IniHelper::ReadInteger("Display", "FontScaling", 1) == 1;
	FontScalingFactor = IniHelper::ReadFloat("Display", "FontScalingFactor", 1.0f);

	// Input
	RawMouseInput = IniHelper::ReadInteger("Input", "RawMouseInput", 1) == 1;
	BlockDirectInputDevices = IniHelper::ReadInteger("Input", "BlockDirectInputDevices", 1) == 1;

	// Graphics
	MaxAnisotropy = IniHelper::ReadInteger("Graphics", "MaxAnisotropy", 16);
	ForceTrilinearFiltering = IniHelper::ReadInteger("Graphics", "ForceTrilinearFiltering", 1) == 1;

	// DLC
	EnableHazardPack = IniHelper::ReadInteger("DLC", "EnableHazardPack", 0) == 1;
	EnableMartialLawPack = IniHelper::ReadInteger("DLC", "EnableMartialLawPack", 0) == 1;
	EnableSupernovaPack = IniHelper::ReadInteger("DLC", "EnableSupernovaPack", 0) == 1;
	EnableSeveredDLC = IniHelper::ReadInteger("DLC", "EnableSeveredDLC", 0) == 1;
	EnableHackerDLC = IniHelper::ReadInteger("DLC", "EnableHackerDLC", 0) == 1;
	EnableZealotDLC = IniHelper::ReadInteger("DLC", "EnableZealotDLC", 0) == 1;
	EnableRivetGunDLC = IniHelper::ReadInteger("DLC", "EnableRivetGunDLC", 0) == 1;

	if (AutoResolution)
	{
		auto [screenWidth, screenHeight] = SystemHelper::GetScreenResolution();
		g_State.screenWidth = screenWidth;
		g_State.screenHeight = screenHeight;
	}

	MaxAnisotropy = std::clamp(MaxAnisotropy, 0, 16);
	IncreasedEntityPersistenceBodies = std::clamp(IncreasedEntityPersistenceBodies, 0, 35);
	IncreasedEntityPersistenceBodies = std::clamp(IncreasedEntityPersistenceBodies, 0, 120);
}

#pragma region Helper

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

	// Reasonable FPS bounds check
	if (target_fps < 1 || target_fps > 1000) 
	{
		return 0.0f;
	}

	float frame_time_ms = 1000.0f / (float)target_fps;
	return frame_time_ms / 2.0f; // Divide by 2 since a1 = 2
}

// Helper function for angle packing
static inline unsigned __int64 PackAngles(float vertical, float horizontal)
{
	unsigned __int64 packed = 0;
	reinterpret_cast<float&>(packed) = vertical;
	reinterpret_cast<float*>(&packed)[1] = horizontal;
	return packed;
}

// Helper function for input scaling
static inline void ScaleRawInput(float rawX, float rawY, float divisor, float& outX, float& outY)
{
	outX = (rawX * g_State.mouseSens) / -divisor;
	outY = (rawY * g_State.mouseSens) / divisor;
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

#pragma endregion

#pragma region Hooks

// =========================
// HavokPhysicsFix
// =========================

safetyhook::InlineHook InitializePhysicsSolverParameters;
safetyhook::InlineHook Build1DAngularConstraintJacobian;
safetyhook::InlineHook SolveBallSocketChainConstraints;
safetyhook::InlineHook BuildContactConstraintJacobian;
safetyhook::InlineHook ProcessEntityDeath;
safetyhook::InlineHook MainLoop;

static void __cdecl BuildContactConstraintJacobian_Hook(__m128* a1, float* a2, bool a3, __m128** a4)
{
	float backup_deltaTime = a2[3];
	float backup_friction = a2[9];

	float timeScale = TARGET_FRAME_TIME / backup_deltaTime;
	a2[3] = backup_deltaTime / timeScale;
	a2[9] = backup_friction / timeScale;

	BuildContactConstraintJacobian.unsafe_call<void>(a1, a2, a3, a4);

	a2[3] = backup_deltaTime;
	a2[9] = backup_friction;
}

static void __cdecl SolveBallSocketChainConstraints_Hook(float* cons, __m128* a2, __m128* a3, __m128** a4)
{
	float rhs_bak = cons[7];
	cons[7] /= g_State.frameTimeScale;
	SolveBallSocketChainConstraints.unsafe_call<void>(cons, a2, a3, a4);
	cons[7] = rhs_bak;
}

static void __cdecl Build1DAngularConstraintJacobian_Hook(__m128* a1, float* cons, __m128** a3)
{
	float rhs_bak = cons[7];
	cons[7] /= g_State.frameTimeScale;
	Build1DAngularConstraintJacobian.unsafe_call<void>(a1, cons, a3);
	cons[7] = rhs_bak;
}

static int __fastcall InitializePhysicsSolverParameters_Hook(float* thisp, int, float* a2, float* a3)
{
	g_State.frameTimeScale = TARGET_FRAME_TIME / a3[2];
	return InitializePhysicsSolverParameters.unsafe_thiscall<int>(thisp, a2, a3);
}

static char __fastcall ProcessEntityDeath_Hook(DWORD* thisp, int, int a2, float a3, int a4, int a5, int a6)
{
	// Ragdoll Death?
	if (a5 == 22)
	{
		g_State.deathFrameCount = 5;
	}

	return ProcessEntityDeath.unsafe_thiscall<char>(thisp, a2, a3, a4, a5, a6);
}

static int __cdecl MainLoop_Hook()
{
	if (g_State.deathFrameCount != 0)
	{
		g_State.deathFrameCount--;
	}

	if (RawMouseInput)
	{
		g_State.frameRawX = g_State.rawMouseDeltaX.exchange(0);
		g_State.frameRawY = g_State.rawMouseDeltaY.exchange(0);
	}

	return MainLoop.unsafe_call<int>();
}

// =========================
// VSyncRefreshRateFix
// =========================

safetyhook::InlineHook SetHz;
safetyhook::InlineHook UpdateDisplaySettings;

static __int16 __cdecl SetHz_Hook(__int16 hz)
{
	MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	return SetHz.call<__int16>(hz);
}

static __int16 __cdecl UpdateDisplaySettings_Hook(__int16 a1, __int16 a2, __int16 hz, char a4, char a5)
{
	MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	return UpdateDisplaySettings.call<__int16>(a1, a2, hz, a4, a5);
}

// =========================
// FixDifficultyRewards
// =========================

safetyhook::InlineHook LoadGame;

static bool __fastcall LoadGame_Hook(DWORD* thisPtr, int, int a2, DWORD* a3, int a4, int a5)
{
	if (a3 && (thisPtr[81] & 0x40))
	{
		// if new game+
		if (MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) && MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) + 0x96C) == 3)
		{
			// write the update flag
			int ng_plus_diff = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) + 0x978);
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr, ng_plus_diff);
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, ng_plus_diff);
		}
		// write the lowest difficulty flag from the save file (or not initialized, used to check for achievements)
		MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, a3[14]);
	}

	return LoadGame.thiscall<bool>(thisPtr, a2, a3, a4, a5);
}

// =========================
// FixSuitIDConflicts
// =========================

safetyhook::InlineHook InitializeItem;

static int __fastcall InitializeItem_hook(DWORD* thisp, int, int a2)
{
	int result = InitializeItem.unsafe_thiscall<int>(thisp, a2);
	DWORD itemId0 = thisp[9];
	DWORD itemId1 = thisp[10];
	DWORD itemId2 = thisp[11];
	DWORD itemId3 = thisp[12];

	// Hacker Suit
	if (itemId0 == 0x58CB43ED && itemId1 == 0xEDE44FA8 && itemId2 == 0x4E4F574B && itemId3 == 0x35373230)
	{
		thisp[151] = 0x317A1E59; // Don't use the unique id of the Elite Advanced Suit
	}

	// Zealot Suit
	else if (itemId0 == 0x58CB5F60 && itemId1 == 0x4BF6F5A0 && itemId2 == 0x574F4843 && itemId3 == 0x39323031)
	{
		thisp[151] = 0x4C79DD58; // Don't use the unique id of the Security Suit
	}

	return result;
}

// =========================
// FixSaveStringHandling
// =========================

safetyhook::InlineHook LoadSaveFileList;
safetyhook::InlineHook CopyStringFromSave;

static int __fastcall LoadSaveFileList_Hook(int thisPtr, int, const wchar_t* Source, int a3, int a4)
{
	// Replace wildcard with two-char pattern for save slots
	if (wcscmp(Source, L"ds_slot_*.deadspacesaved") == 0)
	{
		Source = L"ds_slot_??.deadspacesaved";
	}
	else if (wcscmp(Source, L"ds_slot_*.deadspace2saved") == 0)
	{
		Source = L"ds_slot_??.deadspace2saved";
	}

	return LoadSaveFileList.thiscall<int>(thisPtr, Source, a3, a4);
}

static errno_t __cdecl CopyStringFromSave_Hook(wchar_t* Destination, wchar_t* Source)
{
	if (Source)
	{
		size_t sourceLen = 0;

		// Scan string length with safety limit
		while (sourceLen < 0x800 && Source[sourceLen] != L'\0')
		{
			sourceLen++;
		}

		// Truncate if too long
		if (sourceLen >= 0x80)
		{
			for (size_t i = 0; i < 0x7F; i++)
			{
				Destination[i] = Source[i];
			}

			Destination[0x7F] = L'\0';
			return 0;
		}

		return CopyStringFromSave.call<errno_t>(Destination, Source);
	}

	// NULL source
	memset(Destination, 0, 0x80 * sizeof(wchar_t));
	return -1;
}

// =========================
// FixAutomaticWeaponFireRate
// =========================

safetyhook::InlineHook CheckFireCooldown;
safetyhook::InlineHook UpdateEngineTimer;

static int __fastcall UpdateEngineTimer_Hook(DWORD* thisp, int, DWORD* a2)
{
	int result = UpdateEngineTimer.unsafe_thiscall<int>(thisp, a2);
	g_State.frameTime = *(float*)0x204D3FC;
	return result;
}

static bool __fastcall CheckFireCooldown_Hook(int thisp, int)
{
	float* fireDelayPtr = (float*)(thisp + 800);
	float originalDelay = *fireDelayPtr;

	// Only apply scaling for automatic weapons
	if (originalDelay <= 0.1f)
	{
		float timeDelta = TARGET_FRAME_TIME - g_State.frameTime;
		float coefficient = 10.0f;
		float scalingFactor = 1.0f + coefficient * timeDelta;
		*fireDelayPtr = originalDelay * scalingFactor;
	}

	bool result = CheckFireCooldown.unsafe_thiscall<bool>(thisp);
	*fireDelayPtr = originalDelay;
	return result;
}

// =========================
// DisableOnlineFeatures
// =========================

safetyhook::InlineHook DisplayUIPopup;

static int __cdecl DisplayUIPopup_Hook(const char* Src, int a2)
{
	if (strcmp(Src, "$dlc_package_scanning") == 0 || strcmp(Src, "$ui_nu00_connectingTitle_mc") == 0)
	{
		return 0;
	}

	return DisplayUIPopup.call<int>(Src, a2);
}

// ==========================
// IncreasedEntityPersistence
// ==========================

safetyhook::InlineHook ResizeEntityBuffer;

static int __fastcall ResizeEntityBuffer_Hook(char* thisp, int, int bufferType, int newLimit)
{
	// The game want to clean up the array
	if (newLimit == 0)
	{
		return ResizeEntityBuffer.thiscall<int>(thisp, bufferType, newLimit);
	}

	if (bufferType == 0) // bodies
	{
		newLimit = IncreasedEntityPersistenceBodies;
	}

	if (bufferType == 1) // limbs
	{
		newLimit = IncreasedEntityPersistenceLimbs;
	}

	return ResizeEntityBuffer.thiscall<int>(thisp, bufferType, newLimit);
}

// ====================
// SkipIntro
// ====================

safetyhook::InlineHook LoadMovie;
safetyhook::InlineHook LoadUIAnimation;

static char __fastcall LoadMovie_Hook(int thisPtr, int, const char* Source, char a3, int a4, int a5, int a6)
{
	// Skip logos video
	if (strstr(Source, "trio_frontend.vp6\x00"))
	{
		Source = "\x00";
		(void)LoadMovie.disable();
	}

	return LoadMovie.thiscall<char>(thisPtr, Source, a3, a4, a5, a6);
}

static int __stdcall LoadUIAnimation_Hook(int id)
{
	// FE66 - skip the animation playing alongside the video
	if (id == 0x46453636)
	{
		(void)LoadUIAnimation.disable();
		return 0;
	}

	return LoadUIAnimation.stdcall<int>(id);
}

// ======================
// AutoResolution
// ======================

safetyhook::InlineHook GetConfigInt;

static int __cdecl GetConfigInt_Hook(const char* Src, int ArgList)
{
	if (strcmp(Src, "Window.Width") == 0)
	{
		ArgList = g_State.screenWidth;
	}
	if (strcmp(Src, "Window.Height") == 0)
	{
		ArgList = g_State.screenHeight;
	}

	return GetConfigInt.call<int>(Src, ArgList);
}

// =====================
// RawMouseInput
// =====================

safetyhook::InlineHook ApplyControlConfiguration;
safetyhook::InlineHook UpdateMenuCursor;
safetyhook::InlineHook UpdateCameraTracking;
safetyhook::InlineHook UpdateZeroGravityCamera;
safetyhook::InlineHook UpdateCameraPosition;
safetyhook::InlineHook UpdateAimingCamera;
safetyhook::InlineHook UpdatePlayerCamera;
safetyhook::InlineHook ApplyCameraRotation;
safetyhook::InlineHook hkGetRawInputData;

static int __stdcall ApplyControlConfiguration_Hook(int a1)
{
	// Get the current mouse sensitivity
	g_State.isXInverted = *(BYTE*)(a1);
	g_State.isYInverted = *(BYTE*)(a1 + 1);
	g_State.mouseSens = *(float*)(a1 + 12);
	if (g_State.mouseSens == 0.0f) g_State.mouseSens = 0.005f; // we still want to use the mouse
	return ApplyControlConfiguration.stdcall<int>(a1);
}

static void __fastcall UpdateMenuCursor_Hook(int thisp, int, float a2)
{
	// Get input manager instance
	int inputManager = *(int*)g_Addresses.InputManagerPtr;

	// Calculate cursor movement delta for X and Y axes
	float menuDeltaX = static_cast<float>(g_State.frameRawX) * g_State.mouseSens * 1.6f;
	float menuDeltaY = static_cast<float>(g_State.frameRawY) * g_State.mouseSens * 1.6f;

	// Write the calculated cursor movement deltas
	*(float*)(inputManager + 1348) = menuDeltaX;
	*(float*)(inputManager + 1352) = menuDeltaY;

	// Check if controller is being used (18 = mouse)
	g_State.isControllerActive = (*(int*)(inputManager + 1400) != 18);

	UpdateMenuCursor.unsafe_fastcall<void>(thisp, a2);

	// Clear deltas after processing
	*(float*)(inputManager + 1348) = 0.0f;
	*(float*)(inputManager + 1352) = 0.0f;
}

static int __fastcall UpdateCameraTracking_Hook(int thisp, float a2)
{
	if (!g_State.isControllerActive)
	{
		a2 = TARGET_FRAME_TIME;
	}
	return UpdateCameraTracking.unsafe_fastcall<int>(thisp, a2);
}

static int __fastcall UpdateZeroGravityCamera_Hook(int thisp, float frametime)
{
	if (g_State.isControllerActive)
	{
		return UpdateZeroGravityCamera.unsafe_fastcall<int>(thisp, frametime);
	}

	float deltaX, deltaY;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 625.0f, deltaX, deltaY);

	// Apply invert controls
	if (g_State.isXInverted)
		deltaX = -deltaX;
	if (g_State.isYInverted)
		deltaY = -deltaY;

	// Convert to angular velocity (radians per second)
	const float SENSITIVITY = 20.0f;

	// Update velocities
	*(float*)(thisp + 124) = deltaX * SENSITIVITY;
	*(float*)(thisp + 128) = deltaY * SENSITIVITY;

	return UpdateZeroGravityCamera.unsafe_fastcall<int>(thisp, TARGET_FRAME_TIME);
}

static int __fastcall UpdateCameraPosition_Hook(int thisp, float a2)
{
	if (!g_State.isControllerActive) 
	{
		// Framerate independant sensitivity
		a2 = 1.0f;
	}

	return UpdateCameraPosition.unsafe_fastcall<int>(thisp, a2);
}

static void __fastcall UpdateAimingCamera_Hook(void** thisp, int, int a2, int a3)
{
	g_State.aimingPassCount = 2;
	UpdateAimingCamera.unsafe_thiscall<void>(thisp, a2, a3);
}

static void __fastcall UpdatePlayerCamera_Hook(int thisp, float a2)
{
	g_State.isInNormalCamera = true;
	UpdatePlayerCamera.unsafe_fastcall<void>(thisp, a2);
	g_State.isInNormalCamera = false;
}

static int __fastcall ApplyCameraRotation_Hook(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
	{
		return ApplyCameraRotation.unsafe_thiscall<int>(thisp, a2, a3, a4);
	}

	const float PITCH_LIMIT_NORMAL = M_PI / 3.0f; // 60 degrees
	const float PITCH_LIMIT_AIM_DOWN = -5.0f * M_PI / 12.0f; // -75 degrees

	// Normal camera movement
	if (g_State.isInNormalCamera)
	{
		float horizontalDelta, verticalDelta;
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 625.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted)
			horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted)
			verticalDelta = -verticalDelta;

		float currentPitch = *(float*)thisp;
		float newPitch = currentPitch + verticalDelta;
		if (newPitch > PITCH_LIMIT_NORMAL)
		{
			verticalDelta = PITCH_LIMIT_NORMAL - currentPitch;
		}
		else if (newPitch < -PITCH_LIMIT_NORMAL)
		{
			verticalDelta = -PITCH_LIMIT_NORMAL - currentPitch;
		}
		return ApplyCameraRotation.unsafe_thiscall<int>(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
	}

	// Aiming - first pass
	if (g_State.aimingPassCount == 2)
	{
		float horizontalDelta, verticalDelta;
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted)
			horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted)
			verticalDelta = -verticalDelta;

		float currentPitch = *(float*)thisp;
		float newPitch = currentPitch + verticalDelta;
		if (newPitch > PITCH_LIMIT_NORMAL)
		{
			verticalDelta = PITCH_LIMIT_NORMAL - currentPitch;
		}
		else if (newPitch < PITCH_LIMIT_AIM_DOWN)
		{
			verticalDelta = PITCH_LIMIT_AIM_DOWN - currentPitch;
		}
		g_State.aimingPassCount--;
		return ApplyCameraRotation.unsafe_thiscall<int>(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
	}

	// Aiming - second pass
	if (g_State.aimingPassCount == 1)
	{
		float horizontalDelta, verticalDelta;
		ScaleRawInput(0.0f, static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isYInverted)
			verticalDelta = -verticalDelta;

		float currentPitch = *(float*)thisp;
		float newPitch = currentPitch + verticalDelta;
		if (newPitch > PITCH_LIMIT_NORMAL)
		{
			verticalDelta = PITCH_LIMIT_NORMAL - currentPitch;
		}
		else if (newPitch < PITCH_LIMIT_AIM_DOWN)
		{
			verticalDelta = PITCH_LIMIT_AIM_DOWN - currentPitch;
		}
		g_State.aimingPassCount--;
		return ApplyCameraRotation.unsafe_thiscall<int>(thisp, PackAngles(verticalDelta, 0.0f), a3, a4);
	}

	return ApplyCameraRotation.unsafe_thiscall<int>(thisp, a2, a3, a4);
}

static UINT WINAPI GetRawInputData_Hook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT result = hkGetRawInputData.unsafe_stdcall<UINT>(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if (result != (UINT)-1 && uiCommand == RID_INPUT && pData != NULL)
	{
		RAWINPUT* raw = (RAWINPUT*)pData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			g_State.rawMouseDeltaX += raw->data.mouse.lLastX;
			g_State.rawMouseDeltaY += raw->data.mouse.lLastY;
		}
	}

	return result;
}

// =========================
// BlockDirectInputDevices
// =========================

safetyhook::InlineHook IsXInputDevice;

static bool __cdecl IsXInputDevice_hook(DWORD* lpddi)
{
	// Skip the expensive verification, we filter out DirectInput devices elsewhere
	return false;
}

// =====================
// MaxAnisotropy
// =====================

static safetyhook::InlineHook InitTextureSampler;

static int __cdecl InitTextureSampler_Hook(int a1, int a2)
{
	int result = InitTextureSampler.unsafe_ccall<int>(a1, a2);

	// Get current filtering flags
	int* flags_ptr = (int*)(a2 + 16);
	int flags = *flags_ptr;
	int filtering_mode = flags & 0x30000;

	// Anisotropic Filtering
	if (MaxAnisotropy > 0)
	{
		// Upgrade point (0x0) or linear (0x20000) filtering to anisotropic
		if (filtering_mode == 0x00000 || filtering_mode == 0x20000)
		{
			*(unsigned char*)(a2 + 37) = 3;              // Anisotropic mip mode
			*(unsigned char*)(a2 + 39) = MaxAnisotropy;  // Max anisotropy level
			*flags_ptr = (flags & ~0x30000) | 0x20000;   // Ensure linear flag is set
		}
	}

	// Trilinear Filtering
	if (ForceTrilinearFiltering)
	{
		unsigned char mip_filter = *(unsigned char*)(a2 + 36);
		if (mip_filter == 1) // Point mip filtering
		{
			*(unsigned char*)(a2 + 36) = 2; // Upgrade to linear mip filtering
		}
	}

	return result;
}

// ==============
// DLC
// ==============

safetyhook::InlineHook OpenShop;
safetyhook::InlineHook IsDlcContentOwned;
safetyhook::InlineHook AddShopItem;

static void __fastcall OpenShop_Hook(DWORD* thisPtr, int)
{
	g_State.isLoadingShopItems = true;
	OpenShop.fastcall<void>(thisPtr);
	g_State.isLoadingShopItems = false;
}

static bool __fastcall IsDlcContentOwned_Hook(int thisPtr, int, DWORD* a2)
{
	// Check for all DLC items
	DWORD itemId0 = a2[0];
	DWORD itemId1 = a2[1];
	DWORD itemId2 = a2[2];
	DWORD itemId3 = a2[3];

	// Most items end with BERTRAM1 (0x54524542, 0x314D4152)
	if (g_State.isLoadingShopItems && itemId2 == 0x54524542 && itemId3 == 0x314D4152)
	{
		if (!EnableHazardPack)
		{
			if ((itemId0 == 0x38CB63F1 && itemId1 == 0xDECD957E) ||  // Hazard Suit
				(itemId0 == 0x38CB673C && itemId1 == 0x22E2241E) ||  // Hazard Line Gun
				(itemId0 == 0x38CB63F1 && itemId1 == 0xFA62DA20) ||  // Shockpoint Suit
				(itemId0 == 0x38CB673E && itemId1 == 0xD90EFC1C) ||  // Shockpoint Ripper
				(itemId0 == 0x38CB63F1 && itemId1 == 0xED1EDDAA) ||  // Triage Suit
				(itemId0 == 0x38CB673D && itemId1 == 0x3B05D03C))    // Triage Javelin Gun
			{
				return true; // Disable Hazard Pack item
			}
		}
		if (!EnableMartialLawPack)
		{
			if ((itemId0 == 0x38CB63F2 && itemId1 == 0x5D6D2A6A) ||  // Bloody Vintage Suit
				(itemId0 == 0x38CB674E && itemId1 == 0xC74BFBAC) ||  // Bloody Flamethrower
				(itemId0 == 0x38CB674F && itemId1 == 0xE63E8BAA) ||  // Bloody Force Gun
				(itemId0 == 0x38CB674F && itemId1 == 0x57B08815) ||  // Bloody Javelin Gun
				(itemId0 == 0x38CB63F2 && itemId1 == 0x42052D6A) ||  // Earthgov Security Suit
				(itemId0 == 0x38CB674B && itemId1 == 0x089B5095) ||  // Earthgov Detonator
				(itemId0 == 0x38CB6749 && itemId1 == 0x647FCDD0) ||  // Earthgov Pulse Rifle
				(itemId0 == 0x38CB6749 && itemId1 == 0xE9D2F65F))    // Earthgov Seeker Rifle
			{
				return true; // Disable Martial Law Pack item
			}
		}
		if (!EnableSupernovaPack)
		{
			if ((itemId0 == 0x38CB63F2 && itemId1 == 0x2BE46F24) ||  // Agility Advanced Suit
				(itemId0 == 0x38CB6747 && itemId1 == 0xBACF1A30) ||  // Agility Plasma Cutter
				(itemId0 == 0x38CB6744 && itemId1 == 0x6F1A30B2) ||  // Agility Rivet Gun
				(itemId0 == 0x38CB6748 && itemId1 == 0x95D508D2) ||  // Agility Pulse Rifle
				(itemId0 == 0x38CB63F2 && itemId1 == 0x508D4590) ||  // Forged Engineering Suit
				(itemId0 == 0x38CB674C && itemId1 == 0x111C7694) ||  // Forged Plasma Cutter
				(itemId0 == 0x38CB674C && itemId1 == 0xC36C1B39) ||  // Forged Line Gun
				(itemId0 == 0x38CB674D && itemId1 == 0x5D53841E) ||  // Forged Ripper
				(itemId0 == 0x38CB63F2 && itemId1 == 0x1C0C50E0) ||  // Heavy Duty Vintage Suit
				(itemId0 == 0x38CB6743 && itemId1 == 0x74D94CAC) ||  // Heavy Duty Contact Beam
				(itemId0 == 0x38CB6741 && itemId1 == 0x9463BFB4) ||  // Heavy Duty Detonator
				(itemId0 == 0x38CB673F && itemId1 == 0xA89B2140))    // Heavy Duty Line Gun
			{
				return true; // Disable Supernova Pack item
			}
		}
	}

	return IsDlcContentOwned.thiscall<bool>(thisPtr, a2);
}

static int __fastcall AddShopItem_Hook(int thisPtr, int, DWORD* a2)
{
	if (g_State.isLoadingShopItems)
	{
		DWORD itemId0 = a2[0];
		DWORD itemId1 = a2[1];
		DWORD itemId2 = a2[2];
		DWORD itemId3 = a2[3];

		bool shouldPatch = false;

		if (EnableHackerDLC)
		{
			shouldPatch = (itemId0 == 0x58CB43ED && itemId1 == 0xEDE44FA8 && itemId2 == 0x4E4F574B && itemId3 == 0x35373230) || // Hacker Suit
				(itemId0 == 0x38CB5536 && itemId1 == 0x7AE1DC66 && itemId2 == 0x54524542 && itemId3 == 0x314D4152);   // Hacker Contact Beam
		}

		if (!shouldPatch && EnableSeveredDLC)
		{
			shouldPatch = (itemId0 == 0x31CB7561 && itemId1 == 0x1B516549 && itemId2 == 0x534F5254 && itemId3 == 0x30314E49) || // Patrol Suit
				(itemId0 == 0x38CB673B && itemId1 == 0x11C92BBC && itemId2 == 0x54524542 && itemId3 == 0x314D4152);   // Patrol Seeker Rifle
		}

		if (!shouldPatch && EnableZealotDLC)
		{
			shouldPatch = (itemId0 == 0x58CB5F60 && itemId1 == 0x4BF6F5A0 && itemId2 == 0x574F4843 && itemId3 == 0x39323031) || // Zealot Suit
				(itemId0 == 0x38CB6733 && itemId1 == 0xE6777830 && itemId2 == 0x54524542 && itemId3 == 0x314D4152);   // Zealot Force Gun
		}

		if (!shouldPatch && EnableRivetGunDLC)
		{
			shouldPatch = (itemId0 == 0x33CB4867 && itemId1 == 0x256213EC && itemId2 == 0x4E4F4F4E && itemId3 == 0x32304E41); // Rivet Gun
		}

		if (shouldPatch)
		{
			// Backup original bytes
			const size_t patchSize = 0x1B;
			uint8_t originalBytes[patchSize];
			memcpy(originalBytes, (void*)g_Addresses.ItemCheckAddress, patchSize);

			// Apply NOP patch to disable ownership verification
			MemoryHelper::MakeNOP(g_Addresses.ItemCheckAddress, patchSize);

			// Call the original function
			int res = AddShopItem.thiscall<int>(thisPtr, a2);

			// Restore original bytes
			MemoryHelper::WriteMemoryRaw(g_Addresses.ItemCheckAddress, originalBytes, patchSize);

			return res;
		}
	}

	return AddShopItem.thiscall<int>(thisPtr, a2);
}

#pragma endregion

#pragma region Patch Init

static void ApplyHavokPhysicsFix()
{
	if (!HavokPhysicsFix) return;

	DWORD addr_BuildContactConstraintJacobian = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC A4 00 00 00 8B 55 08", "BuildContactConstraintJacobian");
	DWORD addr_SolveBallSocketChainConstraints = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 84 00 00 00 F3 0F 10 41 04", "SolveBallSocketChainConstraints");
	DWORD addr_Build1DAngularConstraintJacobian = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 14 8B 45 08 53 8B 5D 10", "Build1DAngularConstraintJacobian");
	DWORD addr_InitializePhysicsSolverParameters = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 D9 80 0C 01 00 00 D9 19", "InitializePhysicsSolverParameters");
	DWORD addr_ProcessEntityDeath = ScanModuleSignature(g_State.GameModule, "53 56 8B F1 8B 4C 24 18 8B C1 32 DB 83 E8 16 0F 84 17 01 00 00", "ProcessEntityDeath");
	DWORD addr_physicsImpulseDamper = ScanModuleSignature(g_State.GameModule, "0F C6 D1 AA 0F C6 DB FF F3 0F 58 D4 0F 28 C8 0F C6 C8 FF F3 0F 5C CA F3 0F 59 CB 0F 28 E1", "physicsImpulseDamper");
	DWORD addr_constraintErrorScaler = ScanModuleSignature(g_State.GameModule, "0F 28 16 0F 59 CA 0F 58 C3 0F 58 C1 8D 4A 10 0F 28 C8 0F C6 C8 55 F3 0F 58 C8 83 C2 20 0F C6 C0 AA F3 0F 58 C1 0F C6 D2 FF F3 0F 5C D0 F3 0F 11 94 24 30 02 00 00", "constraintErrorScaler");
	DWORD addr_constraintMassCapture = ScanModuleSignature(g_State.GameModule, "D9 44 24 10 DE FA D9 C9 D9 58 0C D9 44 24 68", "constraintMassCapture");

	if (addr_BuildContactConstraintJacobian == 0 ||
		addr_SolveBallSocketChainConstraints == 0 ||
		addr_Build1DAngularConstraintJacobian == 0 ||
		addr_InitializePhysicsSolverParameters == 0 ||
		addr_ProcessEntityDeath == 0 ||
		addr_physicsImpulseDamper == 0 ||
		addr_constraintErrorScaler == 0 ||
		addr_constraintMassCapture == 0) {
		return;
	}

	BuildContactConstraintJacobian = HookHelper::CreateHook((void*)addr_BuildContactConstraintJacobian, &BuildContactConstraintJacobian_Hook);
	SolveBallSocketChainConstraints = HookHelper::CreateHook((void*)addr_SolveBallSocketChainConstraints, &SolveBallSocketChainConstraints_Hook);
	Build1DAngularConstraintJacobian = HookHelper::CreateHook((void*)addr_Build1DAngularConstraintJacobian, &Build1DAngularConstraintJacobian_Hook);
	InitializePhysicsSolverParameters = HookHelper::CreateHook((void*)addr_InitializePhysicsSolverParameters, &InitializePhysicsSolverParameters_Hook);
	ProcessEntityDeath = HookHelper::CreateHook((void*)addr_ProcessEntityDeath, &ProcessEntityDeath_Hook);

	static SafetyHookMid physicsImpulseDamper{};
	physicsImpulseDamper = safetyhook::create_mid(addr_physicsImpulseDamper,
		[](safetyhook::Context& ctx)
		{
			if (g_State.deathFrameCount > 0 && ctx.xmm3.f32[3] > 0.8f)
			{
				ctx.xmm3.f32[3] = ctx.xmm3.f32[3] / g_State.frameTimeScale;
			}
			else if (ctx.xmm3.f32[3] > 20.0f)
			{
				ctx.xmm3.f32[3] = ctx.xmm3.f32[3] / g_State.frameTimeScale;
			}
		}
	);

	static SafetyHookMid constraintErrorScaler{};
	constraintErrorScaler = safetyhook::create_mid(addr_constraintErrorScaler,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm2.f32[3] = ctx.xmm2.f32[3] / g_State.frameTimeScale;
		}
	);

	static SafetyHookMid constraintMassCapture{};
	constraintMassCapture = safetyhook::create_mid(addr_constraintMassCapture,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp = ctx.esp;
			float* v306_ptr = (float*)(esp + 0x10);
			g_State.constraintMass = *v306_ptr;
		}
	);

	static SafetyHookMid constraintForceDamper{};
	constraintForceDamper = safetyhook::create_mid(addr_constraintMassCapture + 0xB,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp = ctx.esp;
			uint32_t eax = ctx.eax;

			float* v369_ptr = (float*)(esp + 0x158);
			float* v370_ptr = (float*)(esp + 0x15C);

			if (fabs(*v369_ptr) > 0.8f)
			{
				float* tau_ptr = (float*)(eax + 0x0C);
				float scaleFactor = 0.8f;

				*tau_ptr = ((*v370_ptr / g_State.frameTimeScale) / g_State.constraintMass) * scaleFactor;
			}
		}
	);

	static SafetyHookMid timestepLimiter{};
	timestepLimiter = safetyhook::create_mid(addr_constraintMassCapture + 0x15,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp_val = ctx.esp;

			float* v307 = (float*)(esp_val + 0x10);
			float* v369 = (float*)(esp_val + 0x158);

			if (fabs(*v369) > 2.0f || (fabs(*v369) > 0.5f && g_State.constraintMass >= 100.0f))
			{
				float* v307 = (float*)(esp_val + 0x10);
				*v307 = -30.0f;
			}
		}
	);
}

static void ApplyHighCoreCPUFix()
{
	if (!HighCoreCPUFix) return;

	DWORD CPUFix = ScanModuleSignature(g_State.GameModule, "8B 5D D8 83 C4 18 33 FF", "CPUFix");

	if (CPUFix == 0) return;

	static SafetyHookMid CPUCrashFix{};
	CPUCrashFix = safetyhook::create_mid(reinterpret_cast<void*>(CPUFix),
		[](safetyhook::Context& ctx) 
		{
			uint32_t ebp = static_cast<uint32_t>(ctx.ebp);
			uint32_t* cpuCount = reinterpret_cast<uint32_t*>(ebp - 0x20);
			if (*cpuCount == 2) 
			{
				uint32_t* currentAffinityMask = reinterpret_cast<uint32_t*>(ebp - 0x24);
				*currentAffinityMask = 0;
			}
		}
	);
}

static void ApplyVSyncRefreshRateFix()
{
	if (!VSyncRefreshRateFix) return;

	DWORD addr_SetHz = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 A3 ?? ?? ?? ?? C3 CC CC CC CC 66 A1", "SetHz");
	DWORD addr_UpdateDisplaySettings = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 8B 4C 24 08 B2 01 66 39 05", "UpdateDisplaySettings");
	DWORD addr_fpsLimiter = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F8 83 EC 20 53 33 DB 56 38", "fpsLimiter");

	if (addr_SetHz == 0 ||
		addr_UpdateDisplaySettings == 0 ||
		addr_fpsLimiter == 0) {
		return;
	}

	SetHz = HookHelper::CreateHook((void*)addr_SetHz, &SetHz_Hook);
	UpdateDisplaySettings = HookHelper::CreateHook((void*)addr_UpdateDisplaySettings, &UpdateDisplaySettings_Hook);
	g_Addresses.TargetFrameTimeMsPtr = MemoryHelper::ReadMemory<int>(addr_fpsLimiter + 0x37);
}

static void ApplyFixDifficultyRewards()
{
	if (!FixDifficultyRewards) return;

	DWORD addr_LoadGame = ScanModuleSignature(g_State.GameModule, "8B ?? 34 85 ?? 74 ?? 83 ?? 6C 09 00 00 03 75", "LoadGame", 3);
	DWORD addr_LoadGameRef = ScanModuleSignature(g_State.GameModule, "8B ?? 34 85 ?? 74 ?? 83 ?? 6C 09 00 00 03 75", "LoadGame");
	DWORD addr_LoadedSaveMemoryPtr = ScanModuleSignature(g_State.GameModule, "89 15 ?? ?? ?? ?? F3 0F 10 ?? 14", "LoadedSaveMemoryPtr");

	if (addr_LoadGame == 0 ||
		addr_LoadGameRef == 0 ||
		addr_LoadedSaveMemoryPtr == 0) {
		return;
	}

	g_Addresses.NgGamePlusPtr = MemoryHelper::ReadMemory<int>(addr_LoadGameRef - 0x4);
	g_Addresses.LoadedSaveMemoryPtr = MemoryHelper::ReadMemory<int>(addr_LoadedSaveMemoryPtr + 0x2);
	LoadGame = HookHelper::CreateHook((void*)addr_LoadGame, &LoadGame_Hook);
}

static void ApplyFixSuitIDConflicts()
{
	if (!FixSuitIDConflicts) return;

	DWORD addr_InitializeItem = ScanModuleSignature(g_State.GameModule, "83 EC 08 55 56 8B F1 57 85 F6 74", "InitializeItem");

	if (addr_InitializeItem == 0) return;

	InitializeItem = HookHelper::CreateHook((void*)addr_InitializeItem, &InitializeItem_hook);
}

static void ApplyFixSaveStringHandling()
{
	if (!FixSaveStringHandling) return;

	DWORD addr_LoadSaveFileList = ScanModuleSignature(g_State.GameModule, "83 EC 18 53 56 57 33 DB 6A 2C 53 8B F1", "LoadSaveFileList");
	DWORD addr_CopyStringFromSave = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 85 C0 74 14 50 8B 44 24 08 68 80 00", "CopyStringFromSave");

	if (addr_LoadSaveFileList == 0 ||
		addr_CopyStringFromSave == 0) {
		return;
	}

	LoadSaveFileList = HookHelper::CreateHook((void*)addr_LoadSaveFileList, &LoadSaveFileList_Hook);
	CopyStringFromSave = HookHelper::CreateHook((void*)addr_CopyStringFromSave, &CopyStringFromSave_Hook);
}

static void ApplyFixAutomaticWeaponFireRate()
{
	if (!FixAutomaticWeaponFireRate) return;

	DWORD addr_UpdateEngineTimer = ScanModuleSignature(g_State.GameModule, "83 EC 10 55 56 57 8B F1 E8", "UpdateEngineTimer");
	DWORD addr_CheckFireCooldown = ScanModuleSignature(g_State.GameModule, "51 8B 81 18 03 00 00 D9 05", "CheckFireCooldown");

	if (addr_UpdateEngineTimer == 0 ||
		addr_CheckFireCooldown == 0) {
		return;
	}

	UpdateEngineTimer = HookHelper::CreateHook((void*)addr_UpdateEngineTimer, &UpdateEngineTimer_Hook);
	CheckFireCooldown = HookHelper::CreateHook((void*)addr_CheckFireCooldown, &CheckFireCooldown_Hook);
	g_Addresses.EngineFrameTimePtr = MemoryHelper::ReadMemory<int>(addr_UpdateEngineTimer + 0x265);
}

static void ApplyDisableOnlineFeatures()
{
	if (!DisableOnlineFeatures) return;

	DWORD addr_DisplayUIPopup = ScanModuleSignature(g_State.GameModule, "83 EC 0C 83 3D ?? ?? ?? ?? ?? 74 5A", "DisplayUIPopup");
	DWORD addr_StartAuth = ScanModuleSignature(g_State.GameModule, "75 0E 8B CF E8 ?? ?? ?? ?? 5F 5E 5B 83", "StartAuth");
	DWORD addr_ShopOfflineMessage = ScanModuleSignature(g_State.GameModule, "74 25 8B 86 F8 0A 00 00", "ShopOfflineMessage");

	if (addr_DisplayUIPopup == 0 ||
		addr_StartAuth == 0 ||
		addr_ShopOfflineMessage == 0) {
		return;
	}

	DisplayUIPopup = HookHelper::CreateHook((void*)addr_DisplayUIPopup, &DisplayUIPopup_Hook);
	MemoryHelper::MakeNOP(addr_StartAuth, 2);
	MemoryHelper::MakeNOP(addr_ShopOfflineMessage, 2);
}

static void ApplyIncreasedEntityPersistence()
{
	if (!IncreasedEntityPersistence) return;

	DWORD addr_ResizeEntityBuffer = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 EC 14 55 56 8D 04 40 8D 2C C1 57", "ResizeEntityBuffer");
	DWORD addr_ResizeEntityBuffer_Init = ScanModuleSignature(g_State.GameModule, "51 53 33 DB 55 56 57 8B F9 89 3D", "EntityBuffer_Init");

	if (addr_ResizeEntityBuffer == 0 ||
		addr_ResizeEntityBuffer_Init == 0) {
		return;
	}

	MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x34, IncreasedEntityPersistenceBodies);
	MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x7A, IncreasedEntityPersistenceBodies);

	MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x86, IncreasedEntityPersistenceLimbs);
	MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x91, IncreasedEntityPersistenceLimbs);
	ResizeEntityBuffer = HookHelper::CreateHook((void*)addr_ResizeEntityBuffer, &ResizeEntityBuffer_Hook);
}

static void ApplySkipIntro()
{
	if (!SkipIntro) return;

	DWORD addr_LoadUIAnimation = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 08 56 E8 ?? ?? ?? ?? 56 E8 ?? ?? ?? ?? 8B C8", "LoadUIAnimation");
	DWORD addr_LoadMovie = ScanModuleSignature(g_State.GameModule, "83 EC 10 53 55 56 33 DB 57 8B F1 88 5C 24 13 E8", "LoadMovie");

	if (addr_LoadUIAnimation == 0 ||
		addr_LoadMovie == 0) {
		return;
	}

	LoadUIAnimation = HookHelper::CreateHook((void*)addr_LoadUIAnimation, &LoadUIAnimation_Hook);
	LoadMovie = HookHelper::CreateHook((void*)addr_LoadMovie, &LoadMovie_Hook);
}

static void ApplyAutoResolution()
{
	if (!AutoResolution) return;

	DWORD addr_GetConfigInt = ScanModuleSignature(g_State.GameModule, "68 00 04 00 00 D9 1D ?? ?? ?? ?? 68", "GetConfigInt");
	addr_GetConfigInt = MemoryHelper::ResolveRelativeAddress(addr_GetConfigInt, 0x11);

	if (addr_GetConfigInt == 0) return;

	GetConfigInt = HookHelper::CreateHook((void*)addr_GetConfigInt, &GetConfigInt_Hook);
}

static void ApplyFontScaling()
{
	if (!FontScaling) return;

	DWORD addr_FontScaling = ScanModuleSignature(g_State.GameModule, "0F 2F C1 76 03 0F 28 C1 F3 0F 10 5E 18", "FontScaling");

	if (addr_FontScaling == 0) return;

	MemoryHelper::MakeNOP(addr_FontScaling, 8);
	MemoryHelper::MakeNOP(addr_FontScaling + 0x35, 5);

	if (FontScalingFactor == 1.0f) return;

	static SafetyHookMid fontScaler1{};
	fontScaler1 = safetyhook::create_mid(addr_FontScaling + 0x8,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm0.f32[0] = ctx.xmm0.f32[0] * FontScalingFactor;
		}
	);

	static SafetyHookMid fontScaler2{};
	fontScaler2 = safetyhook::create_mid(addr_FontScaling + 0x3A,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm0.f32[0] = ctx.xmm0.f32[0] * FontScalingFactor;
		}
	);
}

static void ApplyRawMouseInput()
{
	if (!RawMouseInput) return;

	DWORD addr_ApplyControlConfiguration = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 08 0F B6 06 50 E8", "ApplyControlConfiguration");
	DWORD addr_UpdateMenuCursor = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 64 53 56 8B F1 F7 46 20 00 00 01 00", "UpdateMenuCursor");
	DWORD addr_UpdateCameraTracking = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 F3 0F 10 45 08 F3 0F 59 05 ?? ?? ?? ?? 81 EC A4 01 00 00", "UpdateCameraTracking");
	DWORD addr_UpdateZeroGravityCamera = ScanModuleSignature(g_State.GameModule, "83 EC 14 53 8B D9 80 BB 94 01 00 00 00", "UpdateZeroGravityCamera_Hook");
	DWORD addr_UpdateCameraPosition = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 F3 0F 10 45 08 D9 45 08", "UpdateCameraPosition");
	DWORD addr_UpdateAimingCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 64 01 00 00 A1 ?? ?? ?? ?? D9 45 0C 53 8B D9", "UpdateAimingCamera");
	DWORD addr_UpdatePlayerCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 34 01 00 00 53 8B D9 8B 43 74", "UpdatePlayerCamera");
	DWORD addr_ApplyCameraRotation = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 54 F3 0F 10 45 08 8B 45", "ApplyCameraRotation");

	if (addr_ApplyControlConfiguration == 0 ||
		addr_UpdateMenuCursor == 0 ||
		addr_UpdateCameraTracking == 0 ||
		addr_UpdateZeroGravityCamera == 0 ||
		addr_UpdateCameraPosition == 0 ||
		addr_UpdateAimingCamera == 0 ||
		addr_UpdatePlayerCamera == 0 ||
		addr_ApplyCameraRotation == 0) {
		return;
	}

	ApplyControlConfiguration = HookHelper::CreateHook((void*)addr_ApplyControlConfiguration, &ApplyControlConfiguration_Hook);
	UpdateMenuCursor = HookHelper::CreateHook((void*)addr_UpdateMenuCursor, &UpdateMenuCursor_Hook);
	UpdateCameraTracking = HookHelper::CreateHook((void*)addr_UpdateCameraTracking, &UpdateCameraTracking_Hook);
	UpdateZeroGravityCamera = HookHelper::CreateHook((void*)addr_UpdateZeroGravityCamera, &UpdateZeroGravityCamera_Hook);
	UpdateCameraPosition = HookHelper::CreateHook((void*)addr_UpdateCameraPosition, &UpdateCameraPosition_Hook);
	UpdateAimingCamera = HookHelper::CreateHook((void*)addr_UpdateAimingCamera, &UpdateAimingCamera_Hook);
	UpdatePlayerCamera = HookHelper::CreateHook((void*)addr_UpdatePlayerCamera, &UpdatePlayerCamera_Hook);
	ApplyCameraRotation = HookHelper::CreateHook((void*)addr_ApplyCameraRotation, &ApplyCameraRotation_Hook);
	hkGetRawInputData = HookHelper::CreateHookAPI(L"user32.dll", "GetRawInputData", &GetRawInputData_Hook);
	g_Addresses.InputManagerPtr = MemoryHelper::ReadMemory<int>(addr_UpdateMenuCursor + 0x29);
}

static void ApplyFilterInputDevices()
{
	if (!BlockDirectInputDevices) return;

	DWORD addr_IsXInputDevice = ScanModuleSignature(g_State.GameModule, "81 EC 84 00 00 00 53 56  57 33 DB 6A 4C", "IsXInputDevice");
	DWORD addr_InitializeInputDevice = ScanModuleSignature(g_State.GameModule, "85 C0 0F 84 6F 01 00 00 8B 40 04 85 C0", "InputDeviceTypeFilter");

	if (addr_IsXInputDevice == 0 ||
		addr_InitializeInputDevice == 0) {
		return;
	}

	IsXInputDevice = HookHelper::CreateHook((void*)addr_IsXInputDevice, &IsXInputDevice_hook);

	static SafetyHookMid InputDeviceTypeFilter{};
	InputDeviceTypeFilter = safetyhook::create_mid(reinterpret_cast<void*>(addr_InitializeInputDevice),
		[](safetyhook::Context& ctx)
		{
			uint32_t eax = static_cast<uint32_t>(ctx.eax);
			if (eax == 0) return;

			uint32_t deviceIndex = *reinterpret_cast<uint32_t*>(eax);
			uint32_t& deviceType = *reinterpret_cast<uint32_t*>(eax + 0x4);

			// Allow mouse, keyboard, and all XInput controllers (slots 0-3)
			bool isAllowed = (deviceType == 2) || // Mouse
				(deviceType == 3) ||              // Keyboard
				(deviceIndex <= 3);               // XInput slots 0-3

			if (!isAllowed)
			{
				deviceType = 4; // Invalid type (skipped)
			}
		}
	);
}

static void ApplyTextureFiltering()
{
	if (MaxAnisotropy == 0 && !ForceTrilinearFiltering) return;

	DWORD addr_InitTextureSampler = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 53 8B 58 10 8B CB 8B D3 81 E1 00 00", "InitTextureSampler");

	if (addr_InitTextureSampler == 0) return;

	InitTextureSampler = HookHelper::CreateHook((void*)addr_InitTextureSampler, &InitTextureSampler_Hook);
}

static void ApplyShopOpenCheck()
{
	if ((EnableHazardPack && EnableMartialLawPack && EnableSupernovaPack) && (!EnableSeveredDLC && !EnableHackerDLC && !EnableZealotDLC && !EnableRivetGunDLC)) return;

	DWORD addr_OpenShop = ScanModuleSignature(g_State.GameModule, "51 53 8B D9 83 BB F8 0A 00 00 00 89 5C 24 04 0F", "OpenShop");

	if (addr_OpenShop == 0) return;

	OpenShop = HookHelper::CreateHook((void*)addr_OpenShop, &OpenShop_Hook);
}

static void ApplyBlockDLCUnlock()
{
	if (EnableHazardPack && EnableMartialLawPack && EnableSupernovaPack) return;

	DWORD addr_IsDlcContentOwned = ScanModuleSignature(g_State.GameModule, "51 8B 41 44 53 55 33 DB 56 57 89 4C 24 10 3B C3", "IsDlcContentOwned");

	if (addr_IsDlcContentOwned == 0) return;

	IsDlcContentOwned = HookHelper::CreateHook((void*)addr_IsDlcContentOwned, &IsDlcContentOwned_Hook);
}

static void ApplyForceDLCUnlock()
{
	if (!EnableSeveredDLC && !EnableHackerDLC && !EnableZealotDLC && !EnableRivetGunDLC) return;

	DWORD addr_AddShopItem = ScanModuleSignature(g_State.GameModule, "83 EC 28 53 55 56 8B 74 24 38 8B 06 57 8B F9 85", "AddShopItem");

	if (addr_AddShopItem == 0) return;

	AddShopItem = HookHelper::CreateHook((void*)addr_AddShopItem, &AddShopItem_Hook);
	g_Addresses.ItemCheckAddress = addr_AddShopItem + 0x175;
}

static void ApplyMainLoopHook()
{
	if (!HavokPhysicsFix && !RawMouseInput) return;

	DWORD addr_MainLoop = ScanModuleSignature(g_State.GameModule, "83 EC 20 56 57 8B 3D ?? ?? ?? ?? 6A 03 33 F6 56", "MainLoop");

	if (addr_MainLoop == 0) return;

	MainLoop = HookHelper::CreateHook((void*)addr_MainLoop, &MainLoop_Hook);
}

static void Init()
{
	ReadConfig();

	// Fixes
	ApplyHavokPhysicsFix();
	ApplyHighCoreCPUFix();
	ApplyVSyncRefreshRateFix();
	ApplyFixDifficultyRewards();
	ApplyFixSuitIDConflicts();
	ApplyFixSaveStringHandling();
	ApplyFixAutomaticWeaponFireRate();

	// General
	ApplyDisableOnlineFeatures();
	ApplyIncreasedEntityPersistence();
	ApplySkipIntro();

	// Display
	ApplyAutoResolution();
	ApplyFontScaling();

	// Input
	ApplyRawMouseInput();
	ApplyFilterInputDevices();

	// Graphics
	ApplyTextureFiltering();

	// DLC
	ApplyShopOpenCheck();
	ApplyBlockDLCUnlock();
	ApplyForceDLCUnlock();

	// Misc
	ApplyMainLoopHook();
}

#pragma endregion

#pragma region Init

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Prevents DLL from receiving thread notifications
			DisableThreadLibraryCalls(hModule);

			// Skip wrapper initialization when loaded as .asi
			if (!IsUALPresent())
			{
				SystemHelper::LoadProxyLibrary();
			}

			regOpenKeyHook = HookHelper::CreateHookAPI(L"advapi32.dll", "RegOpenKeyExW", &RegOpenKeyExW_Hook);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}
	return TRUE;
}

#pragma endregion