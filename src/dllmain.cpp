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
#include "helper.hpp"

#include "AchievementOverlay.hpp"

#include "Shaders/GlassReflectionVS.hpp"

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

	float frame_time_ms = 1000.0f / (float)target_fps;
	return frame_time_ms / 2.0f; // Divide by 2 since a1 = 2
}

// Helper function for angle packing
static inline unsigned __int64 PackAngles(float vertical, float horizontal)
{
	float angles[2] = { vertical, horizontal };
	return std::bit_cast<std::uint64_t>(angles);
}

// Helper function for input scaling
static inline void ScaleRawInput(float rawX, float rawY, float divisor, float& outX, float& outY)
{
	outX = (rawX * g_State.mouseSens) / -divisor;
	outY = (rawY * g_State.mouseSens) / divisor;
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

static void ReleaseFlareFixResources()
{
	if (g_State.proxySurf)
	{
		g_State.proxySurf->Release();
		g_State.proxySurf = nullptr;
	}

	if (g_State.proxyTex)
	{
		g_State.proxyTex->Release();
		g_State.proxyTex = nullptr;
	}

	g_State.sourceTex = nullptr;

	g_State.resourcesValid = false;
	g_State.snapshotValid = false;
}

static bool TrackSourceTexture(IDirect3DBaseTexture9* src)
{
	IDirect3DDevice9* dev = GetD3D9Device();
	if (!src || !dev) return false;
	if (src->GetType() != D3DRTYPE_TEXTURE) return false;
	if (g_State.resourcesValid && g_State.sourceTex == src) return true;

	IDirect3DTexture9* src2d = static_cast<IDirect3DTexture9*>(src);
	D3DSURFACE_DESC desc{};
	if (FAILED(src2d->GetLevelDesc(0, &desc))) return false;

	ReleaseFlareFixResources();

	HRESULT hr = dev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &g_State.proxyTex, nullptr);
	if (FAILED(hr)) return false;

	hr = g_State.proxyTex->GetSurfaceLevel(0, &g_State.proxySurf);
	if (FAILED(hr))
	{
		ReleaseFlareFixResources(); return false;
	}

	g_State.sourceTex = src;
	g_State.resourcesValid = true;
	g_State.snapshotValid = false;
	return true;
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

	if (AchievementSupport)
	{
		AchievementOverlay::Update(GetD3D9Device());
	}

	return MainLoop.unsafe_ccall<int>();
}

// =========================
// VSyncRefreshRateFix
// =========================

safetyhook::InlineHook SetHz;

static __int16 __cdecl SetHz_Hook(__int16 hz)
{
	MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	return SetHz.ccall<__int16>(hz);
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
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr, ng_plus_diff, false);
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, ng_plus_diff, false);
		}
		// write the lowest difficulty flag from the save file (or not initialized, used to check for achievements)
		MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, a3[14], false);
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

	// Hacker Suit
	if (MatchId(thisp + 9, 0x58CB43ED, 0xEDE44FA8, 0x4E4F574B, 0x35373230))
	{
		thisp[151] = 0x317A1E59; // Don't use the unique id of the Elite Advanced Suit
	}
	// Zealot Suit
	else if (MatchId(thisp + 9, 0x58CB5F60, 0x4BF6F5A0, 0x574F4843, 0x39323031))
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
	if (!Source)
	{
		// NULL source
		if (Destination)
		{
			memset(Destination, 0, 0x80);
		}

		return -1;
	}

	if (!Destination)
	{
		return -1;
	}

	size_t i;
	for (i = 0; i < 0x7F; i++)
	{
		if (Source[i] == L'\0')
		{
			Destination[i] = L'\0';
			return 0;
		}

		Destination[i] = Source[i];
	}

	Destination[0x7F] = L'\0';
	return 0;
}

// =========================
// FixAutomaticWeaponFireRate
// =========================

safetyhook::InlineHook CheckFireCooldown;
safetyhook::InlineHook UpdateEngineTimer;

static int __fastcall UpdateEngineTimer_Hook(DWORD* thisp, int, DWORD* a2)
{
	int result = UpdateEngineTimer.unsafe_thiscall<int>(thisp, a2);
	g_State.frameTime = MemoryHelper::ReadMemory<float>(g_Addresses.EngineFrameTimePtr);
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
// FixSolarArrayElevator
// =========================

safetyhook::InlineHook Timeline_ScheduleEvent;

static char __fastcall Timeline_ScheduleEvent_Hook(int* thisp, int, DWORD* a2, int a3, float a4, unsigned int a5, int a6)
{
	if (a2 && a2[0] == 0x716F8DC9)
	{
		g_State.elevatorFixArmed = true;
	}

	return Timeline_ScheduleEvent.unsafe_thiscall<char>(thisp, a2, a3, a4, a5, a6);
}

// =========================
// FixBlurResolution
// =========================

safetyhook::InlineHook RenderBokehBlur;

static int __cdecl RenderBokehBlur_Hook(int a1, int a2, float cocRadius, float a4, char a5, int a6, int a7, float a8, float a9)
{
	if (g_State.currentHeight > 720)
	{
		cocRadius *= g_State.resolutionScale;
	}

	return RenderBokehBlur.unsafe_ccall<int>(a1, a2, cocRadius, a4, a5, a6, a7, a8, a9);
}

// =========================
// FixShadowBlur
// =========================

safetyhook::InlineHook IterativeShadowBlur;

static int __cdecl IterativeShadowBlur_Hook(unsigned int a1, float a2, float a3, float a4, float a5, int passCount, char a7)
{
	if (g_State.currentHeight > 720)
	{
		passCount = static_cast<int>(static_cast<float>(passCount) * g_State.resolutionScale * g_State.resolutionScale);
	}

	return IterativeShadowBlur.unsafe_ccall<int>(a1, a2, a3, a4, a5, passCount, a7);
}

// =========================
// FixFlareArtifacts
// =========================

static safetyhook::InlineHook RenderFlare;
static uintptr_t RenderFlare_Trampoline = 0;

__declspec(naked) static int __cdecl RenderFlare_Hook(DWORD* pFlare, int textureId, float posX, float posY, float sizeX, float sizeY, float rotation, float alpha, float colorReg, int renderPass, bool isScreenSpace)
{
	__asm
	{
		mov byte ptr[g_State.inFlareDraw], 1

		// Forward stack args, offset stays 0x28 as ESP drops
		push dword ptr[esp + 0x28] // isScreenSpace
		push dword ptr[esp + 0x28] // renderPass
		push dword ptr[esp + 0x28] // colorReg
		push dword ptr[esp + 0x28] // alpha
		push dword ptr[esp + 0x28] // rotation
		push dword ptr[esp + 0x28] // sizeY
		push dword ptr[esp + 0x28] // sizeX
		push dword ptr[esp + 0x28] // posY
		push dword ptr[esp + 0x28] // posX
		push dword ptr[esp + 0x28] // textureId

		// Call original function
		mov edx, [RenderFlare_Trampoline]
		call edx

		// Balance stack
		add esp, 0x28

		mov byte ptr[g_State.inFlareDraw], 0
		ret
	}
}

// =========================
// AchievementSupport
// =========================

safetyhook::InlineHook GetGameLanguage;
safetyhook::InlineHook TrophyCountUpdate;
safetyhook::InlineHook GiveTrophy;
safetyhook::InlineHook UpdateObtainedTrophy;
safetyhook::InlineHook LoadTrophyCounter;

static int __cdecl GetGameLanguage_Hook(char* String2, size_t MaxCount)
{
	int result = GetGameLanguage.ccall<int>(String2, MaxCount);

	switch (result)
	{
		case 2: AchievementOverlay::SetLanguage("fr"); break;
		case 3: AchievementOverlay::SetLanguage("de"); break;
		case 5: AchievementOverlay::SetLanguage("it"); break;
		case 8: AchievementOverlay::SetLanguage("es"); break;
		default: AchievementOverlay::SetLanguage("en"); break;
	}

	return result;
}

static char __fastcall TrophyCountUpdate_Hook(int thisPtr, int, unsigned int* a2)
{
	char result = TrophyCountUpdate.unsafe_thiscall<char>(thisPtr, a2);

	int slot = AchievementOverlay::CounterSlotByHash(*a2);
	if (slot >= 0)
	{
		AchievementOverlay::UpdateCounterByHash(*a2, *(int*)(thisPtr + 20 + slot * 4));
	}

	return result;
}

static void __fastcall GiveTrophy_Hook(BYTE* thisPtr, int, unsigned int a2)
{
	GiveTrophy.unsafe_thiscall<void>(thisPtr, a2);

	if (AchievementOverlay::NotifyUnlock((int)a2))
	{
		*(thisPtr + 24) |= 1;
		GiveTrophy.unsafe_thiscall<void>(thisPtr, 0);
	}
}

static char __fastcall UpdateObtainedTrophy_Hook(char* thisPtr, int, unsigned int a2, char a3, char a4)
{
	char result = UpdateObtainedTrophy.unsafe_thiscall<char>(thisPtr, a2, a3, a4);
	AchievementOverlay::SetAchievementUnlocked((int)a2, a3 != 0);
	return result;
}

static char __fastcall LoadTrophyCounter_Hook(int thisPtr, int, int a2, const void* a3, int a4, int a5)
{
	char result = LoadTrophyCounter.unsafe_thiscall<char>(thisPtr, a2, a3, a4, a5);
	if (a3 && a4)
	{
		const int* counters = (const int*)a3;

		for (int slot = 0; slot < 38; slot++)
		{
			AchievementOverlay::InitCounterBySlot(slot, counters[slot]);
		}
	}

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

	return DisplayUIPopup.ccall<int>(Src, a2);
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

	if (bufferType == 0 && IncreasedEntityPersistenceBodies != 0) // bodies
	{
		newLimit = IncreasedEntityPersistenceBodies;
	}

	if (bufferType == 1 && IncreasedEntityPersistenceLimbs != 0) // limbs
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

	return GetConfigInt.ccall<int>(Src, ArgList);
}

// =====================
// RawMouseInput
// =====================

safetyhook::InlineHook ApplyControlConfiguration;
safetyhook::InlineHook UpdateMenuCursor;
safetyhook::InlineHook UpdateCameraTracking;
safetyhook::InlineHook UpdateZeroGravityCamera;
safetyhook::InlineHook UpdateCameraPosition;
safetyhook::InlineHook hkGetRawInputData;
safetyhook::InlineHook UpdateAimWithMomentum;
safetyhook::InlineHook UpdateBoundedAim;
safetyhook::InlineHook UpdateConeAim;
safetyhook::InlineHook UpdateOscillatingAim;
safetyhook::InlineHook UpdateWeaponPoseBlend;
static int(__thiscall* OriginalApplyCameraRotation)(int*, unsigned __int64, unsigned int, int) = nullptr;

static int __stdcall ApplyControlConfiguration_Hook(int a1)
{
	// Get the current mouse sensitivity
	g_State.isXInverted = *(BYTE*)(a1);
	g_State.isYInverted = *(BYTE*)(a1 + 1);
	g_State.mouseSens = *(float*)(a1 + 12);
	if (g_State.mouseSens == 0.0f) g_State.mouseSens = 0.005f; // we still want to use the mouse
	ControllerHelper::SetGyroInvertX(g_State.isXInverted);
	ControllerHelper::SetGyroInvertY(g_State.isYInverted);
	return ApplyControlConfiguration.stdcall<int>(a1);
}

static void __fastcall UpdateMenuCursor_Hook(int thisp, float a2)
{
	// Get input manager instance
	int inputManager = *(int*)g_Addresses.InputManagerPtr;

	// Check if controller is being used (18 = mouse)
	g_State.isControllerActive = (*(int*)(inputManager + 1400) != 18);

	UpdateMenuCursor.unsafe_fastcall<void>(thisp, a2);
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

static int __fastcall UpdateWeaponPoseBlend_Hook(int* thisp, int, float a2)
{
	if (!g_State.isControllerActive)
	{
		// Framerate independant sensitivity
		a2 = TARGET_FRAME_TIME;
	}

	return UpdateWeaponPoseBlend.unsafe_thiscall<int>(thisp, a2);
}

static int __fastcall UpdateAimWithMomentum_Hook(int* thisp, int)
{
	uintptr_t self = reinterpret_cast<uintptr_t>(thisp);
	int v2 = *reinterpret_cast<int*>(self + 116);
	uintptr_t v3 = (v2 != 0 && v2 != 16) ? *reinterpret_cast<uintptr_t*>(self + 124) : 0;

	g_State.momentumAimOuter = thisp;
	g_State.momentumAimData = v3;
	int result = UpdateAimWithMomentum.unsafe_thiscall<int>(thisp);
	g_State.momentumAimOuter = nullptr;
	g_State.momentumAimData = 0;
	return result;
}

static int __fastcall UpdateBoundedAim_Hook(int* thisp, int)
{
	g_State.boundedAimOuter = thisp;
	int result = UpdateBoundedAim.unsafe_thiscall<int>(thisp);
	g_State.boundedAimOuter = nullptr;
	return result;
}

static int __fastcall UpdateConeAim_Hook(int* thisp, int)
{
	uintptr_t self = reinterpret_cast<uintptr_t>(thisp);
	uintptr_t wrapper = *reinterpret_cast<uintptr_t*>(self + 20);
	uintptr_t data = 0;
	if (wrapper)
	{
		uintptr_t base = wrapper - 16;
		if (base)
		{
			data = *reinterpret_cast<uintptr_t*>(base + 12);
		}
	}

	g_State.coneAimData = data;
	int result = UpdateConeAim.unsafe_fastcall<int>(thisp);
	g_State.coneAimData = 0;
	return result;
}

static int __fastcall UpdateOscillatingAim_Hook(int* thisp, int)
{
	g_State.oscillatingAimOuter = thisp;
	int result = UpdateOscillatingAim.unsafe_fastcall<int>(thisp);
	g_State.oscillatingAimOuter = nullptr;
	return result;
}

static int __fastcall AimingApplyRotation_Primary(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
	{
		if (ControllerHelper::IsGyroEnabled())
		{
			float gyroYaw, gyroPitch;
			ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

			if (gyroYaw != 0.0f || gyroPitch != 0.0f)
			{
				float angles[2];
				std::memcpy(angles, &a2, sizeof(angles));
				angles[0] += gyroPitch;
				angles[1] += gyroYaw;

				float currentPitch = *(float*)thisp;
				float newPitch = currentPitch + angles[0];
				angles[0] = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

				return OriginalApplyCameraRotation(thisp, PackAngles(angles[0], angles[1]), a3, a4);
			}
		}

		return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

	if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
}

static int __fastcall AimingApplyRotation_Secondary(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(0.0f, static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, 0.0f), a3, a4);
}

static int __fastcall PlayerApplyRotation(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 625.0f, horizontalDelta, verticalDelta);

	if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, -PITCH_LIMIT_NORMAL, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
}

static int __fastcall ApplyAimWithMomentum(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.momentumAimOuter;
	uintptr_t v3 = g_State.momentumAimData;

	if (!outer || !v3)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	float rollDelta = 0.0f;
	bool momentumAlreadyApplied = false;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float gyroYaw, gyroPitch;
		ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

		if (gyroYaw == 0.0f && gyroPitch == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float angles[2];
		std::memcpy(angles, &a2, sizeof(angles));
		verticalDelta = angles[0] + gyroPitch;
		horizontalDelta = angles[1] + gyroYaw;

		std::memcpy(&rollDelta, &a3, sizeof(rollDelta));
		momentumAlreadyApplied = true;
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	float pitchMinAbs = *reinterpret_cast<float*>(v3 + 108) * DEG2RAD;
	float pitchMax = *reinterpret_cast<float*>(v3 + 112) * DEG2RAD;
	float yawMax = *reinterpret_cast<float*>(v3 + 116) * DEG2RAD;
	float yawMinAbs = *reinterpret_cast<float*>(v3 + 120) * DEG2RAD;

	float* snapshot = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisp) + 160);
	float currentPitch = snapshot[0];
	float currentYaw = snapshot[1];

	float newPitch = std::clamp(currentPitch + verticalDelta, -pitchMinAbs, pitchMax);
	float newYaw = std::clamp(currentYaw + horizontalDelta, -yawMinAbs, yawMax);

	float pitchDelta = newPitch - currentPitch;
	float yawDelta = newYaw - currentYaw;

	if (!momentumAlreadyApplied)
	{
		uintptr_t v3Class = *reinterpret_cast<uintptr_t*>(v3);
		if (v3Class && *reinterpret_cast<unsigned char*>(v3Class + 137) != 0)
		{
			uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);
			float v224 = *reinterpret_cast<float*>(outerAddr + 224);
			float v228 = *reinterpret_cast<float*>(outerAddr + 228);
			float rollYawCoef = MemoryHelper::ReadMemory<float>(g_Addresses.CameraRollToYawCoef);

			pitchDelta += v224;
			yawDelta += v228 * rollYawCoef;
			rollDelta = v228;
		}
	}

	unsigned int rollBits;
	std::memcpy(&rollBits, &rollDelta, sizeof(rollBits));
	return OriginalApplyCameraRotation(thisp, PackAngles(pitchDelta, yawDelta), rollBits, a4);
}

static int __fastcall ApplyBoundedAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.boundedAimOuter;
	if (!outer)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		ControllerHelper::GetProcessedGyroDelta(horizontalDelta, verticalDelta);

		if (horizontalDelta == 0.0f && verticalDelta == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	const uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);

	uintptr_t wrapper = *reinterpret_cast<uintptr_t*>(outerAddr + 112);
	if (!wrapper)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	uintptr_t v4 = wrapper - 16;
	uintptr_t clampBase;
	switch (*reinterpret_cast<int*>(v4 + 588))
	{
		case 1:     clampBase = v4 + 608; break;
		case 2:
		case 3:     clampBase = v4 + 624; break;
		case 5:     clampBase = v4 + 640; break;
		default:    clampBase = v4 + 592; break;
	}

	float* bounds = reinterpret_cast<float*>(clampBase);
	float pitchMax = bounds[0] * DEG2RAD;
	float pitchMin = bounds[1] * DEG2RAD;
	float yawMax = bounds[2] * DEG2RAD;
	float yawMin = bounds[3] * DEG2RAD;

	float* storedPitch = reinterpret_cast<float*>(outerAddr + 96);
	float* storedYaw = reinterpret_cast<float*>(outerAddr + 100);

	float newPitch = std::clamp(*storedPitch + verticalDelta, pitchMin, pitchMax);
	float newYaw = std::clamp(*storedYaw + horizontalDelta, yawMin, yawMax);

	*storedPitch = newPitch;
	*storedYaw = newYaw;

	return OriginalApplyCameraRotation(thisp, PackAngles(newPitch, newYaw), a3, a4);
}

static int __fastcall ApplyConeAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	uintptr_t d = g_State.coneAimData;
	if (!d)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float gyroYaw, gyroPitch;
		ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

		if (gyroYaw == 0.0f && gyroPitch == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float angles[2];
		std::memcpy(angles, &a2, sizeof(angles));
		verticalDelta = angles[0] + gyroPitch;
		horizontalDelta = angles[1] + gyroYaw;
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	float pitchMinAbs = *reinterpret_cast<float*>(d + 92);
	float pitchMax = *reinterpret_cast<float*>(d + 96);
	float yawMaxLow = *reinterpret_cast<float*>(d + 100);
	float yawMinAbs = *reinterpret_cast<float*>(d + 104);
	float pitchThresh = *reinterpret_cast<float*>(d + 108);
	float yawMaxHigh = *reinterpret_cast<float*>(d + 112);

	float* cam = reinterpret_cast<float*>(thisp);
	float currentPitch = cam[0];
	float currentYaw = cam[1];

	float newPitch = std::clamp(currentPitch + verticalDelta, -pitchMinAbs, pitchMax);

	float yawMax;
	float yawRange = yawMaxHigh - yawMaxLow;
	if (newPitch >= pitchThresh && yawRange != 0.0f)
	{
		float slope = (pitchMax - pitchThresh) / yawRange;
		if (slope != 0.0f)
		{
			float intercept = pitchThresh - slope * yawMaxLow;
			yawMax = (newPitch - intercept) / slope;
		}
		else
		{
			yawMax = yawMaxLow;
		}
	}
	else
	{
		yawMax = yawMaxLow;
	}

	float newYaw = std::clamp(currentYaw + horizontalDelta, -yawMinAbs, yawMax);

	return OriginalApplyCameraRotation(thisp, PackAngles(newPitch - currentPitch, newYaw - currentYaw), a3, a4);
}

static int __fastcall ApplyOscillatingAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.oscillatingAimOuter;
	if (!outer)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		ControllerHelper::GetProcessedGyroDelta(horizontalDelta, verticalDelta);

		if (horizontalDelta == 0.0f && verticalDelta == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	const uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);

	float yawMin = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownYawMin);
	float yawMax = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownYawMax);
	float pitchMin = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownPitchMin);
	float pitchMax = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownPitchMax);
	float rollYawCoef = MemoryHelper::ReadMemory<float>(g_Addresses.CameraRollToYawCoef);

	float* storedPitch = reinterpret_cast<float*>(outerAddr + 120);
	float* storedYaw = reinterpret_cast<float*>(outerAddr + 124);
	float pitchBob = *reinterpret_cast<float*>(outerAddr + 128);
	float rollBob = *reinterpret_cast<float*>(outerAddr + 132);

	float newPitch = std::clamp(*storedPitch + verticalDelta, pitchMin, pitchMax);
	float newYaw = std::clamp(*storedYaw + horizontalDelta, yawMin, yawMax);

	*storedPitch = newPitch;
	*storedYaw = newYaw;
	float packedPitch = newPitch + pitchBob;
	float packedYaw = (rollBob * rollYawCoef) + newYaw;
	unsigned int rollBits;
	std::memcpy(&rollBits, &rollBob, sizeof(rollBits));
	return OriginalApplyCameraRotation(thisp, PackAngles(packedPitch, packedYaw), rollBits, a4);
}

static UINT WINAPI GetRawInputData_Hook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT result = hkGetRawInputData.unsafe_stdcall<UINT>(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if (result != (UINT)-1 && uiCommand == RID_INPUT && pData != NULL)
	{
		RAWINPUT* raw = (RAWINPUT*)pData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			if (AchievementOverlay::IsVisible())
			{
				// Overlay open: hide movement and clicks from the game
				raw->data.mouse.lLastX = 0;
				raw->data.mouse.lLastY = 0;
				raw->data.mouse.usButtonFlags = 0;
			}
			else
			{
				g_State.rawMouseDeltaX += raw->data.mouse.lLastX;
				g_State.rawMouseDeltaY += raw->data.mouse.lLastY;
			}
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

// =========================
// UseSDLControllerInput
// =========================

safetyhook::InlineHook XInputGetStateHook;
safetyhook::InlineHook XInputSetStateHook;

static DWORD WINAPI XInputGetState_Hook(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	if (dwUserIndex != 0) return ERROR_DEVICE_NOT_CONNECTED;
	DWORD result = ControllerHelper::PollController(pState, InvertABXYButtons);

	if (AchievementSupport && pState)
	{
		AchievementOverlay::FeedControllerState(*pState, result == ERROR_SUCCESS);

		if (AchievementOverlay::WantCaptureController())
		{
			ZeroMemory(&pState->Gamepad, sizeof(XINPUT_GAMEPAD)); // hide input from the game
		}
	}

	return result;
}

static DWORD WINAPI XInputSetState_Hook(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
	if (dwUserIndex != 0) return ERROR_DEVICE_NOT_CONNECTED;
	return ControllerHelper::SetVibration(pVibration);
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

// =========================
// DynamicShadowResolution
// =========================

static safetyhook::InlineHook SetDynamicShadowMapResolution;

static unsigned int __cdecl SetDynamicShadowMapResolution_Hook(int shadowRes)
{
	if (shadowRes == 1920)
	{
		shadowRes = DynamicShadowResolution;
	}

	return SetDynamicShadowMapResolution.ccall<unsigned int>(shadowRes);
}

// ==============
// DLC
// ==============

safetyhook::InlineHook OpenShop;
safetyhook::InlineHook AddShopItem;
safetyhook::InlineHook IsDlcContentOwned;
safetyhook::InlineHook DlcRegistered;
safetyhook::InlineHook DlcOwnershipCheck;
safetyhook::InlineHook DlcStatusQuery;

static bool IsForcedItem(const DWORD* item)
{
	if (EnableHackerDLC)
	{
		if (MatchId(item, 0x58CB43ED, 0xEDE44FA8, 0x4E4F574B, 0x35373230) || // Hacker Suit
			MatchId(item, 0x38CB5536, 0x7AE1DC66, 0x54524542, 0x314D4152))   // Hacker Contact Beam
			return true;
	}
	if (EnableSeveredDLC)
	{
		if (MatchId(item, 0x31CB7561, 0x1B516549, 0x534F5254, 0x30314E49) || // Patrol Suit
			MatchId(item, 0x38CB673B, 0x11C92BBC, 0x54524542, 0x314D4152))   // Patrol Seeker Rifle
			return true;
	}
	if (EnableZealotDLC)
	{
		if (MatchId(item, 0x58CB5F60, 0x4BF6F5A0, 0x574F4843, 0x39323031) || // Zealot Suit
			MatchId(item, 0x38CB6733, 0xE6777830, 0x54524542, 0x314D4152))   // Zealot Force Gun
			return true;
	}
	if (EnableRivetGunDLC)
	{
		if (MatchId(item, 0x33CB4867, 0x256213EC, 0x4E4F4F4E, 0x32304E41))   // Rivet Gun
			return true;
	}

	return false;
}

static bool IsBlockedItem(const DWORD* item)
{
	if (item[2] != 0x54524542 || item[3] != 0x314D4152) // BERTRAM1
		return false;

	if (!EnableHazardPack)
	{
		if (MatchId(item, 0x38CB63F1, 0xDECD957E, 0x54524542, 0x314D4152) ||  // Hazard Suit
			MatchId(item, 0x38CB673C, 0x22E2241E, 0x54524542, 0x314D4152) ||  // Hazard Line Gun
			MatchId(item, 0x38CB63F1, 0xFA62DA20, 0x54524542, 0x314D4152) ||  // Shockpoint Suit
			MatchId(item, 0x38CB673E, 0xD90EFC1C, 0x54524542, 0x314D4152) ||  // Shockpoint Ripper
			MatchId(item, 0x38CB63F1, 0xED1EDDAA, 0x54524542, 0x314D4152) ||  // Triage Suit
			MatchId(item, 0x38CB673D, 0x3B05D03C, 0x54524542, 0x314D4152))    // Triage Javelin Gun
			return true;
	}
	if (!EnableMartialLawPack)
	{
		if (MatchId(item, 0x38CB63F2, 0x5D6D2A6A, 0x54524542, 0x314D4152) ||  // Bloody Vintage Suit
			MatchId(item, 0x38CB674E, 0xC74BFBAC, 0x54524542, 0x314D4152) ||  // Bloody Flamethrower
			MatchId(item, 0x38CB674F, 0xE63E8BAA, 0x54524542, 0x314D4152) ||  // Bloody Force Gun
			MatchId(item, 0x38CB674F, 0x57B08815, 0x54524542, 0x314D4152) ||  // Bloody Javelin Gun
			MatchId(item, 0x38CB63F2, 0x42052D6A, 0x54524542, 0x314D4152) ||  // Earthgov Security Suit
			MatchId(item, 0x38CB674B, 0x089B5095, 0x54524542, 0x314D4152) ||  // Earthgov Detonator
			MatchId(item, 0x38CB6749, 0x647FCDD0, 0x54524542, 0x314D4152) ||  // Earthgov Pulse Rifle
			MatchId(item, 0x38CB6749, 0xE9D2F65F, 0x54524542, 0x314D4152))    // Earthgov Seeker Rifle
			return true;
	}
	if (!EnableSupernovaPack)
	{
		if (MatchId(item, 0x38CB63F2, 0x2BE46F24, 0x54524542, 0x314D4152) ||  // Agility Advanced Suit
			MatchId(item, 0x38CB6747, 0xBACF1A30, 0x54524542, 0x314D4152) ||  // Agility Plasma Cutter
			MatchId(item, 0x38CB6744, 0x6F1A30B2, 0x54524542, 0x314D4152) ||  // Agility Rivet Gun
			MatchId(item, 0x38CB6748, 0x95D508D2, 0x54524542, 0x314D4152) ||  // Agility Pulse Rifle
			MatchId(item, 0x38CB63F2, 0x508D4590, 0x54524542, 0x314D4152) ||  // Forged Engineering Suit
			MatchId(item, 0x38CB674C, 0x111C7694, 0x54524542, 0x314D4152) ||  // Forged Plasma Cutter
			MatchId(item, 0x38CB674C, 0xC36C1B39, 0x54524542, 0x314D4152) ||  // Forged Line Gun
			MatchId(item, 0x38CB674D, 0x5D53841E, 0x54524542, 0x314D4152) ||  // Forged Ripper
			MatchId(item, 0x38CB63F2, 0x1C0C50E0, 0x54524542, 0x314D4152) ||  // Heavy Duty Vintage Suit
			MatchId(item, 0x38CB6743, 0x74D94CAC, 0x54524542, 0x314D4152) ||  // Heavy Duty Contact Beam
			MatchId(item, 0x38CB6741, 0x9463BFB4, 0x54524542, 0x314D4152) ||  // Heavy Duty Detonator
			MatchId(item, 0x38CB673F, 0xA89B2140, 0x54524542, 0x314D4152))    // Heavy Duty Line Gun
			return true;
	}

	return false;
}

static void __fastcall OpenShop_Hook(DWORD* thisPtr, int)
{
	g_State.isLoadingShopItems = true;
	OpenShop.unsafe_fastcall<void>(thisPtr);
	g_State.isLoadingShopItems = false;
}

static bool __stdcall DlcRegistered_Hook(int a1)
{
	if (g_State.forceCurrentItem)
		return true;

	return DlcRegistered.unsafe_stdcall<bool>(a1);
}

static char __fastcall DlcStatusQuery_Hook(DWORD* thisPtr, int, unsigned int a2, BYTE* a3)
{
	if (g_State.forceCurrentItem)
	{
		*a3 = 1;
		return 1;
	}

	return DlcStatusQuery.unsafe_thiscall<char>(thisPtr, a2, a3);
}

static char __stdcall DlcOwnershipCheck_Hook(int a1)
{
	if (g_State.forceCurrentItem)
		return 1;

	return DlcOwnershipCheck.unsafe_stdcall<char>(a1);
}

static bool __fastcall IsDlcContentOwned_Hook(int thisPtr, int, DWORD* a2)
{
	if (!g_State.isLoadingShopItems)
		return IsDlcContentOwned.unsafe_thiscall<bool>(thisPtr, a2);

	if (g_State.forceCurrentItem)
		return false;

	if (IsBlockedItem(a2))
		return true;

	return IsDlcContentOwned.unsafe_thiscall<bool>(thisPtr, a2);
}

static int __fastcall AddShopItem_Hook(int thisPtr, int, DWORD* a2)
{
	if (g_State.isLoadingShopItems && IsForcedItem(a2))
	{
		DWORD* shop = (DWORD*)thisPtr;
		DWORD origLevel = shop[14];
		DWORD origTier = shop[15];
		shop[14] = 0x7FFFFFFF;
		shop[15] = 0x7FFFFFFF;

		g_State.forceCurrentItem = true;
		int res = AddShopItem.thiscall<int>(thisPtr, a2);
		g_State.forceCurrentItem = false;

		shop[14] = origLevel;
		shop[15] = origTier;
		return res;
	}

	return AddShopItem.thiscall<int>(thisPtr, a2);
}

// ==================
// Resolution Util
// ==================

safetyhook::InlineHook SetResolution;
safetyhook::InlineHook UpdateDisplaySettings;

static __int16 __cdecl SetResolution_Hook(__int16 width, __int16 height)
{
	g_State.currentHeight = height;
	g_State.resolutionScale = static_cast<float>(height) / 720.0f;
	return SetResolution.ccall<__int16>(width, height);
}

static __int16 __cdecl UpdateDisplaySettings_Hook(__int16 width, __int16 height, __int16 hz, char a4, char a5)
{
	g_State.currentHeight = height;
	g_State.resolutionScale = static_cast<float>(height) / 720.0f;

	if (VSyncRefreshRateFix)
	{
		MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	}

	return UpdateDisplaySettings.ccall<__int16>(width, height, hz, a4, a5);
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
	DWORD addr_fpsLimiter = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F8 83 EC 20 53 33 DB 56 38", "fpsLimiter");

	if (addr_SetHz == 0 ||
		addr_fpsLimiter == 0) {
		return;
	}

	SetHz = HookHelper::CreateHook((void*)addr_SetHz, &SetHz_Hook);
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

static void ApplyFixSolarArrayElevator()
{
	if (!FixSolarArrayElevator) return;

	DWORD addr_Timeline_ScheduleEvent = ScanModuleSignature(g_State.GameModule, "0F 57 C0 83 EC 24 0F 2F 44 24 30", "Timeline_ScheduleEvent");
	DWORD addr_ElevatorFlushHook = ScanModuleSignature(g_State.GameModule, "85 C0 75 4F E8", "ElevatorFlushHook");

	if (addr_Timeline_ScheduleEvent == 0 ||
		addr_ElevatorFlushHook == 0) {
		return;
	}

	Timeline_ScheduleEvent = HookHelper::CreateHook((void*)addr_Timeline_ScheduleEvent, &Timeline_ScheduleEvent_Hook);

	static SafetyHookMid ElevatorFlushHook{};
	ElevatorFlushHook = safetyhook::create_mid(reinterpret_cast<void*>(addr_ElevatorFlushHook),
		[](safetyhook::Context& ctx)
		{
			if (!g_State.elevatorFixArmed) return;

			if (MemoryHelper::ReadMemory<uint32_t>(ctx.esi + 0xC) == 0x808075B6)
			{
				ctx.ebp = 0x7FFFFFFF;
				g_State.elevatorFixArmed = false;
			}
		}
	);
}

static void ApplyFixBlurResolution()
{
	if (!FixBlurResolution) return;

	DWORD addr_BlurResolution = ScanModuleSignature(g_State.GameModule, "51 E8 ?? ?? ?? FF 50 8D 4C 24 04 E8", "BlurResolution");

	if (addr_BlurResolution == 0) return;

	RenderBokehBlur = HookHelper::CreateHook((void*)(addr_BlurResolution), &RenderBokehBlur_Hook);
}

static void ApplyFixShadowBlur()
{
	if (!FixShadowBlur) return;

	DWORD addr_ShadowBlur = ScanModuleSignature(g_State.GameModule, "83 EC 44 53 56 57 83 F8 07 73 0A B8 07 00 00 00 A3", "ShadowBlur");

	if (addr_ShadowBlur == 0) return;

	IterativeShadowBlur = HookHelper::CreateHook((void*)(addr_ShadowBlur - 0xB), &IterativeShadowBlur_Hook);
}

static void ApplyFixFlareArtifacts()
{
	if (!FixFlareArtifacts) return;

	DWORD addr_RenderFlare = ScanModuleSignature(g_State.GameModule, "56 8B F0 8B 06 57 BF 01 00 00 00 23 C7", "RenderFlare");
	DWORD addr_FlareSnapshot = ScanModuleSignature(g_State.GameModule, "83 C0 28 88 4E 1E 2B D5 8B 36 3B D3 0F 85", "FlareSnapshot");
	DWORD addr_FlareTextureSubst = ScanModuleSignature(g_State.GameModule, "8B 10 51 6A 04 53 50 8B 82 14 01 00 00 FF D0", "FlareTextureSubst");
	DWORD addr_DeviceCleanupPre = ScanModuleSignature(g_State.GameModule, "85 C0 74 12 8B 08 8B 51 08 50 FF D2 C7 05 ?? ?? ?? ?? ?? ?? ?? ?? E9", "DeviceCleanupPre");

	if (addr_RenderFlare == 0 ||
		addr_FlareSnapshot == 0 ||
		addr_FlareTextureSubst == 0 ||
		addr_DeviceCleanupPre == 0) {
		return;
	}

	RenderFlare = HookHelper::CreateHook((void*)addr_RenderFlare, &RenderFlare_Hook);
	RenderFlare_Trampoline = RenderFlare.trampoline().address();

	static SafetyHookMid FlareSnapshot{};
	FlareSnapshot = safetyhook::create_mid(reinterpret_cast<void*>(addr_FlareSnapshot + 0x2B),
		[](safetyhook::Context&)
		{
			if (!g_State.resourcesValid || !g_State.sourceTex || !g_State.proxySurf) return;

			IDirect3DDevice9* dev = GetD3D9Device();
			if (!dev) return;

			IDirect3DTexture9* src2d = static_cast<IDirect3DTexture9*>(g_State.sourceTex);
			IDirect3DSurface9* srcSurf = nullptr;
			if (FAILED(src2d->GetSurfaceLevel(0, &srcSurf)) || !srcSurf) return;

			HRESULT hr = dev->StretchRect(srcSurf, nullptr, g_State.proxySurf, nullptr, D3DTEXF_NONE);
			srcSurf->Release();

			g_State.snapshotValid = SUCCEEDED(hr);
		}
	);

	static SafetyHookMid FlareTextureSubst{};
	FlareTextureSubst = safetyhook::create_mid(reinterpret_cast<void*>(addr_FlareTextureSubst + 0x1B),
		[](safetyhook::Context& ctx)
		{
			if (ctx.ebx != 258) return;
			if (!g_State.inFlareDraw) return;

			IDirect3DBaseTexture9* tex = reinterpret_cast<IDirect3DBaseTexture9*>(ctx.ecx);
			if (!tex) return;

			if (!g_State.resourcesValid || g_State.sourceTex != tex)
			{
				TrackSourceTexture(tex);
			}

			if (!g_State.snapshotValid) return;
			if (g_State.sourceTex != tex) return;

			ctx.ecx = reinterpret_cast<uintptr_t>(g_State.proxyTex);
		}
	);

	static SafetyHookMid DeviceCleanupPre{};
	DeviceCleanupPre = safetyhook::create_mid(reinterpret_cast<void*>(addr_DeviceCleanupPre - 0x3B),
		[](safetyhook::Context&)
		{
			ReleaseFlareFixResources();
			g_State.device = nullptr;
		}
	);
}

static void ApplyFixGlassReflections()
{
	if (!FixGlassReflections) return;

	DWORD addr_VSTable = ScanModuleSignature(g_State.GameModule, "00 00 0F 80 01 00 00 02 00 08 2F 80 00 00 55 A0", "GlassVSTable");

	if (addr_VSTable == 0) return;

	uint32_t fixedVSPtr = (uint32_t)(uintptr_t)g_GlassReflectionVS;

	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x144, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x148, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x14C, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x154, fixedVSPtr);
}

static void ApplyAchievementSupport()
{
	if (!AchievementSupport) return;

	DWORD addr_GetGameLanguage = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 0C 57 8B 7C 24 0C 56 57 68", "GetGameLanguage");
	DWORD addr_TrophyCountUpdate = ScanModuleSignature(g_State.GameModule, "8B 15 ?? ?? ?? ?? 83 EC 20 53 33 DB 56 8B F1", "TrophyCountUpdate");
	DWORD addr_GiveTrophy = ScanModuleSignature(g_State.GameModule, "80 79 10 00 74 3A 8B 44 24 04", "GiveTrophy");
	DWORD addr_UpdateObtainedTrophy = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 F8 40 73 33 8D 44 40 06", "UpdateObtainedTrophy");
	DWORD addr_LoadTrophyCounter = ScanModuleSignature(g_State.GameModule, "83 7C 24 0C 00 75 18 68 98 00 00 00", "LoadTrophyCounter");

	if (addr_GetGameLanguage == 0 ||
		addr_TrophyCountUpdate == 0 ||
		addr_GiveTrophy == 0 ||
		addr_UpdateObtainedTrophy == 0 ||
		addr_LoadTrophyCounter == 0) {
		return;
	}

	GetGameLanguage = HookHelper::CreateHook((void*)addr_GetGameLanguage, &GetGameLanguage_Hook);
	TrophyCountUpdate = HookHelper::CreateHook((void*)addr_TrophyCountUpdate, &TrophyCountUpdate_Hook);
	GiveTrophy = HookHelper::CreateHook((void*)addr_GiveTrophy, &GiveTrophy_Hook);
	UpdateObtainedTrophy = HookHelper::CreateHook((void*)addr_UpdateObtainedTrophy, &UpdateObtainedTrophy_Hook);
	LoadTrophyCounter = HookHelper::CreateHook((void*)addr_LoadTrophyCounter, &LoadTrophyCounter_Hook);
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

	if (IncreasedEntityPersistenceBodies != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x34, IncreasedEntityPersistenceBodies);
		MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x7A, IncreasedEntityPersistenceBodies);
	}

	if (IncreasedEntityPersistenceLimbs != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x86, IncreasedEntityPersistenceLimbs);
		MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x91, IncreasedEntityPersistenceLimbs);
	}

	ResizeEntityBuffer = HookHelper::CreateHook((void*)addr_ResizeEntityBuffer, &ResizeEntityBuffer_Hook);
}

static void ApplyIncreasedDecalPersistence()
{
	if (!IncreasedDecalPersistence) return;

	DWORD addr_DecalVertexBuffer = ScanModuleSignature(g_State.GameModule, "51 6A 10 68 00 53 07 00", "DecalVertexBuffer");
	DWORD addr_CreateVertexBuffer1 = ScanModuleSignature(g_State.GameModule, "6A 00 89 76 08 6A 00 C7 46 04 00 00 10 00", "CreateVertexBuffer1");
	DWORD addr_CreateVertexBuffer2 = ScanModuleSignature(g_State.GameModule, "53 89 76 08 53 C7 46 04 00 00 10 00", "CreateVertexBuffer2");
	DWORD addr_VertexBufferSize = ScanModuleSignature(g_State.GameModule, "00 00 10 00 2B C2 3B C6 B9 00 10 00 00", "VertexBufferSize");

	if (addr_DecalVertexBuffer == 0 ||
		addr_CreateVertexBuffer1 == 0 ||
		addr_CreateVertexBuffer2 == 0 ||
		addr_VertexBufferSize == 0) {
		return;
	}

	MemoryHelper::WriteMemory<int>(addr_DecalVertexBuffer + 0x4, 1920000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0xA, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0x22, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x8, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x20, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_VertexBufferSize, 0x400000);
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

static void ApplySkipArtificialLoadingDelay()
{
	if (!SkipArtificialLoadingDelay) return;

	DWORD addr_SkipArtificialLoadingDelay = ScanModuleSignature(g_State.GameModule, "72 29 85 C9 74 E7", "SkipArtificialLoadingDelay");

	if (addr_SkipArtificialLoadingDelay == 0) return;

	MemoryHelper::MakeNOP(addr_SkipArtificialLoadingDelay, 2);
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
	DWORD addr_FontScaling2 = ScanModuleSignature(g_State.GameModule, "76 03 0F 28 C2 0F B7 0D", "FontScaling2");

	if (addr_FontScaling == 0 ||
		addr_FontScaling2 == 0) {
		return;
	}

	MemoryHelper::MakeNOP(addr_FontScaling, 8);
	MemoryHelper::MakeNOP(addr_FontScaling + 0x35, 5);

	MemoryHelper::MakeNOP(addr_FontScaling2, 5);
	MemoryHelper::MakeNOP(addr_FontScaling2 + 0x2F, 5);

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
	DWORD addr_UpdateWeaponPoseBlend = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 F3 0F 10 45 08 F3 0F 59 05 ?? ?? ?? ?? 81 EC A4 01 00 00", "UpdateWeaponPoseBlend");
	DWORD addr_UpdateAimWithMomentum = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 74 53 56 8B F1 8B 46 74", "UpdateAimWithMomentum");
	DWORD addr_UpdateBoundedAim = ScanModuleSignature(g_State.GameModule, "C3 CC 83 EC 20 D9 05 ?? ?? ?? ?? 53 56 D9 5C 24 08", "UpdateBoundedAim");
	DWORD addr_UpdateConeAim = ScanModuleSignature(g_State.GameModule, "83 EC 30 56 8B F1 8B 46 14 85 C0 0F 84", "UpdateConeAim");
	DWORD addr_UpdateOscillatingAim = ScanModuleSignature(g_State.GameModule, "83 EC 08 F3 0F 10 05 ?? ?? ?? ?? 53 56 57 8B F9", "UpdateOscillatingAim");

	if (addr_ApplyControlConfiguration == 0 ||
		addr_UpdateMenuCursor == 0 ||
		addr_UpdateCameraTracking == 0 ||
		addr_UpdateZeroGravityCamera == 0 ||
		addr_UpdateCameraPosition == 0 ||
		addr_UpdateAimingCamera == 0 ||
		addr_UpdatePlayerCamera == 0 ||
		addr_ApplyCameraRotation == 0 ||
		addr_UpdateWeaponPoseBlend == 0 ||
		addr_UpdateAimWithMomentum == 0 || 
		addr_UpdateBoundedAim == 0 ||
		addr_UpdateConeAim == 0 ||
		addr_UpdateOscillatingAim == 0) {
		return;
	}

	ApplyControlConfiguration = HookHelper::CreateHook((void*)addr_ApplyControlConfiguration, &ApplyControlConfiguration_Hook);
	UpdateMenuCursor = HookHelper::CreateHook((void*)addr_UpdateMenuCursor, &UpdateMenuCursor_Hook);
	UpdateCameraTracking = HookHelper::CreateHook((void*)addr_UpdateCameraTracking, &UpdateCameraTracking_Hook);
	UpdateZeroGravityCamera = HookHelper::CreateHook((void*)addr_UpdateZeroGravityCamera, &UpdateZeroGravityCamera_Hook);
	UpdateCameraPosition = HookHelper::CreateHook((void*)addr_UpdateCameraPosition, &UpdateCameraPosition_Hook);
	UpdateWeaponPoseBlend = HookHelper::CreateHook((void*)addr_UpdateWeaponPoseBlend, &UpdateWeaponPoseBlend_Hook);
	UpdateAimWithMomentum = HookHelper::CreateHook((void*)addr_UpdateAimWithMomentum, &UpdateAimWithMomentum_Hook);
	UpdateBoundedAim = HookHelper::CreateHook((void*)(addr_UpdateBoundedAim + 0x2), &UpdateBoundedAim_Hook);
	UpdateConeAim = HookHelper::CreateHook((void*)addr_UpdateConeAim, &UpdateConeAim_Hook);
	UpdateOscillatingAim = HookHelper::CreateHook((void*)addr_UpdateOscillatingAim, &UpdateOscillatingAim_Hook);

	OriginalApplyCameraRotation = reinterpret_cast<decltype(OriginalApplyCameraRotation)>(addr_ApplyCameraRotation);
	MemoryHelper::MakeCALL(addr_UpdateAimingCamera + 0xEE, (uintptr_t)&AimingApplyRotation_Primary);
	MemoryHelper::MakeCALL(addr_UpdateAimingCamera + 0x18F, (uintptr_t)&AimingApplyRotation_Secondary);
	MemoryHelper::MakeCALL(addr_UpdatePlayerCamera + 0x7AE, (uintptr_t)&PlayerApplyRotation);
	MemoryHelper::MakeCALL(addr_UpdateAimWithMomentum + 0x4E0, (uintptr_t)&ApplyAimWithMomentum);
	MemoryHelper::MakeCALL(addr_UpdateBoundedAim + 0x206, (uintptr_t)&ApplyBoundedAim);
	MemoryHelper::MakeCALL(addr_UpdateConeAim + 0x2DC, (uintptr_t)&ApplyConeAim);
	MemoryHelper::MakeCALL(addr_UpdateOscillatingAim + 0x244, (uintptr_t)&ApplyOscillatingAim);

	hkGetRawInputData = HookHelper::CreateHookAPI(L"user32.dll", "GetRawInputData", &GetRawInputData_Hook);

	g_Addresses.InputManagerPtr = MemoryHelper::ReadMemory<int>(addr_UpdateMenuCursor + 0x29);
	g_Addresses.UpsideDownYawMin = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x18C);
	g_Addresses.UpsideDownYawMax = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1A8);
	g_Addresses.UpsideDownPitchMin = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1BD);
	g_Addresses.UpsideDownPitchMax = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1CA);
	g_Addresses.CameraRollToYawCoef = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x214);
}

static void ApplyFilterInputDevices()
{
	if (!BlockDirectInputDevices) return;

	DWORD addr_IsXInputDevice = ScanModuleSignature(g_State.GameModule, "81 EC 84 00 00 00 53 56 57 33 DB 6A 4C", "IsXInputDevice");
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

static void ApplyUseSDLControllerInput()
{
	if (!UseSDLControllerInput) return;

	ControllerHelper::InitializeSDLGamepad();
	XInputGetStateHook = HookHelper::CreateHookAPI(L"XINPUT1_3.dll", "XInputGetState", &XInputGetState_Hook);
	XInputSetStateHook = HookHelper::CreateHookAPI(L"XINPUT1_3.dll", "XInputSetState", &XInputSetState_Hook);
}

static void ApplyTextureFiltering()
{
	if (MaxAnisotropy == 0 && !ForceTrilinearFiltering) return;

	DWORD addr_InitTextureSampler = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 53 8B 58 10 8B CB 8B D3 81 E1 00 00", "InitTextureSampler");

	if (addr_InitTextureSampler == 0) return;

	InitTextureSampler = HookHelper::CreateHook((void*)addr_InitTextureSampler, &InitTextureSampler_Hook);
}

static void ApplyDynamicShadowResolution()
{
	if (DynamicShadowResolution <= 1920) return;

	DWORD addr_ShadowRes = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 E0 E0 83 EC 0C 3B 05", "ShadowRes");

	if (addr_ShadowRes == 0) return;

	SetDynamicShadowMapResolution = HookHelper::CreateHook((void*)addr_ShadowRes, &SetDynamicShadowMapResolution_Hook);
}

static void ApplyShopHooks()
{
	bool needBlock = !EnableHazardPack || !EnableMartialLawPack || !EnableSupernovaPack;
	bool needForce = EnableSeveredDLC || EnableHackerDLC || EnableZealotDLC || EnableRivetGunDLC;

	if (!needBlock && !needForce) return;

	DWORD addr_OpenShop = ScanModuleSignature(g_State.GameModule, "51 53 8B D9 83 BB F8 0A 00 00 00 89 5C 24 04 0F", "OpenShop");
	DWORD addr_AddShopItem = ScanModuleSignature(g_State.GameModule, "83 EC 28 53 55 56 8B 74 24 38 8B 06 57 8B F9 85", "AddShopItem");
	DWORD addr_IsDlcContentOwned = ScanModuleSignature(g_State.GameModule, "51 8B 41 44 53 55 33 DB 56 57 89 4C 24 10 3B C3", "IsDlcContentOwned");

	if (!addr_OpenShop || !addr_AddShopItem || !addr_IsDlcContentOwned) return;

	OpenShop = HookHelper::CreateHook((void*)addr_OpenShop, &OpenShop_Hook);
	AddShopItem = HookHelper::CreateHook((void*)addr_AddShopItem, &AddShopItem_Hook);
	IsDlcContentOwned = HookHelper::CreateHook((void*)addr_IsDlcContentOwned, &IsDlcContentOwned_Hook);

	if (needForce)
	{
		DWORD addr_DlcRegistered = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 50 E8 ?? ?? ?? 00 33 C9 83 F8 FF", "DlcRegistered");
		DWORD addr_DlcOwnershipCheck = ScanModuleSignature(g_State.GameModule, "53 B3 01 E8 ?? ?? ?? ?? 83 F8 04 75", "DlcOwnershipCheck");
		DWORD addr_DlcStatusQuery = ScanModuleSignature(g_State.GameModule, "CC CC CC CC CC CC CC CC 8B 54 24 04 32 C0 3B 91 18 03 00 00", "DlcStatusQuery");

		if (!addr_DlcRegistered || !addr_DlcOwnershipCheck || !addr_DlcStatusQuery) return;

		DlcRegistered = HookHelper::CreateHook((void*)addr_DlcRegistered, &DlcRegistered_Hook);
		DlcOwnershipCheck = HookHelper::CreateHook((void*)addr_DlcOwnershipCheck, &DlcOwnershipCheck_Hook);
		DlcStatusQuery = HookHelper::CreateHook((void*)(addr_DlcStatusQuery + 0x8), &DlcStatusQuery_Hook);
	}
}

static void ApplyMainLoopHook()
{
	if (!HavokPhysicsFix && !RawMouseInput && !AchievementSupport) return;

	DWORD addr_MainLoop = ScanModuleSignature(g_State.GameModule, "83 EC 20 56 57 8B 3D ?? ?? ?? ?? 6A 03 33 F6 56", "MainLoop");

	if (addr_MainLoop == 0) return;

	MainLoop = HookHelper::CreateHook((void*)addr_MainLoop, &MainLoop_Hook);
}

static void ApplyResolutionHook()
{
	if (!FixBlurResolution && !FixShadowBlur && !VSyncRefreshRateFix) return;

	DWORD addr_UpdateDisplaySettings = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 8B 4C 24 08 B2 01 66 39 05", "UpdateDisplaySettings");
	DWORD addr_SetResolution = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 8B 4C 24 08 66 A3", "SetResolution");

	if (addr_UpdateDisplaySettings == 0 ||
		addr_SetResolution == 0) {
		return;
	}

	UpdateDisplaySettings = HookHelper::CreateHook((void*)addr_UpdateDisplaySettings, &UpdateDisplaySettings_Hook);
	SetResolution = HookHelper::CreateHook((void*)addr_SetResolution, &SetResolution_Hook);
}

static void ApplyD3D9ApiMidHook()
{
	if (!FixFlareArtifacts && !AchievementSupport) return;

	DWORD addr_ResetSite1 = ScanModuleSignature(g_State.GameModule, "8B 42 40 FF D0 83 3D", "ResetSite1");
	DWORD addr_ResetSite2 = ScanModuleSignature(g_State.GameModule, "8B 08 8B 51 40 83 C4 04 68", "ResetSite2");

	if (addr_ResetSite1 == 0 ||
		addr_ResetSite2 == 0) {
		return;
	}

	g_Addresses.DevicePtr = MemoryHelper::ReadMemory<int>(addr_ResetSite2 - 0x4);

	static SafetyHookMid ResetSite1Pre{};
	ResetSite1Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1),
		[](safetyhook::Context&)
		{
			if (FixFlareArtifacts)
			{
				ReleaseFlareFixResources();
				g_State.device = nullptr;
			}

			if (AchievementSupport)
			{
				AchievementOverlay::OnDeviceLost();
			}
		}
	);

	static SafetyHookMid ResetSite2Pre{};
	ResetSite2Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 - 0x5),
		[](safetyhook::Context&)
		{
			if (FixFlareArtifacts)
			{
				ReleaseFlareFixResources();
				g_State.device = nullptr;
			}

			if (AchievementSupport)
			{
				AchievementOverlay::OnDeviceLost();
			}
		}
	);

	if (!AchievementSupport) return;

	AchievementOverlay::Init(g_Addresses.DevicePtr);

	DWORD addr_Present1 = ScanModuleSignature(g_State.GameModule, "8B 51 44 6A 00 6A 00 6A 00 6A 00", "Present1");
	DWORD addr_Present2 = ScanModuleSignature(g_State.GameModule, "8B 51 44 56 56 56 56 50", "Present2");

	if (addr_Present1 == 0 ||
		addr_Present2 == 0) {
		return;
	}

	static SafetyHookMid Present1{};
	Present1 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present1),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnPresent();
		}
	);
	
	static SafetyHookMid Present2{};
	Present2 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present2),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnPresent();
		}
	);

	static SafetyHookMid ResetSite1Post{};
	ResetSite1Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1 + 0x5),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnDeviceReset();
		}
	);

	static SafetyHookMid ResetSite2Post{};
	ResetSite2Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 + 0x10),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnDeviceReset();
		}
	);
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
			if (UseSDLControllerInput)
			{
				ControllerHelper::ShutdownSDLGamepad();
			}
			break;
		}
	}
	return TRUE;
}

#pragma endregion