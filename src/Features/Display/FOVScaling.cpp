#pragma once

#include "../../Globals.cpp"

// =========================
// FOVScaling
// =========================

safetyhook::InlineHook CameraManager_SetRenderCamera;
safetyhook::InlineHook CameraManager_GetActiveCamFov;
safetyhook::InlineHook PlayerFallSM_PushTrapCam;
safetyhook::InlineHook PlayerTransitionToGravitySM_PushLandingCam;
safetyhook::InlineHook Sentient_SpawnObserverPoleCamera;
safetyhook::InlineHook PairedAttackCoordinatorSM_PushCamera;
safetyhook::InlineHook PoleCamera_Init;

static float ScaleFOV(float fov)
{
	return 2.0f * atanf(tanf(fov * 0.5f) * FOVScale);
}

static int __fastcall CameraManager_SetRenderCamera_Hook(DWORD* thisp, int, unsigned int viewportIndex, float* pos, float* rot, float fov, float nearZ, float farZ)
{
	return CameraManager_SetRenderCamera.unsafe_thiscall<int>(thisp, viewportIndex, pos, rot, ScaleFOV(fov), nearZ, farZ);
}

static float __fastcall CameraManager_GetActiveCamFov_Hook(DWORD* thisp, int, unsigned int playerID)
{
	float fov = CameraManager_GetActiveCamFov.unsafe_thiscall<float>(thisp, playerID);

	// Cameras copying the active one keep its original FOV, it gets scaled again once rendered
	if (g_State.isCopyingCamera) return fov;

	return ScaleFOV(fov);
}

static int __fastcall PlayerFallSM_PushTrapCam_Hook(DWORD* thisp, int)
{
	g_State.isCopyingCamera = true;
	int result = PlayerFallSM_PushTrapCam.unsafe_thiscall<int>(thisp);
	g_State.isCopyingCamera = false;
	return result;
}

static void __fastcall PlayerTransitionToGravitySM_PushLandingCam_Hook(DWORD* thisp, int)
{
	g_State.isCopyingCamera = true;
	PlayerTransitionToGravitySM_PushLandingCam.unsafe_thiscall<void>(thisp);
	g_State.isCopyingCamera = false;
}

static int __fastcall Sentient_SpawnObserverPoleCamera_Hook(DWORD* thisp, int, unsigned int viewportId)
{
	g_State.isCopyingCamera = true;
	int result = Sentient_SpawnObserverPoleCamera.unsafe_thiscall<int>(thisp, viewportId);
	g_State.isCopyingCamera = false;
	return result;
}

static int __fastcall PairedAttackCoordinatorSM_PushCamera_Hook(DWORD* thisp, int)
{
	g_State.isCopyingCamera = true;
	int result = PairedAttackCoordinatorSM_PushCamera.unsafe_thiscall<int>(thisp);
	g_State.isCopyingCamera = false;
	return result;
}

static int __fastcall PoleCamera_Init_Hook(DWORD* thisp, int, unsigned int playerID, bool resetCamera)
{
	g_State.isCopyingCamera = true;
	int result = PoleCamera_Init.unsafe_thiscall<int>(thisp, playerID, resetCamera);
	g_State.isCopyingCamera = false;
	return result;
}

static void ApplyFOVScaling()
{
	if (FOVScale == 1.0f) return;

	DWORD addr_CameraManager_SetRenderCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 8C 00 00 00 83 B9 64 01 00 00 00", "CameraManager_SetRenderCamera");
	DWORD addr_CameraManager_GetActiveCamFov = ScanModuleSignature(g_State.GameModule, "80 B9 E4 00 00 00 00 74 0F D9 81 50 01 00 00 D8 0D", "CameraManager_GetActiveCamFov");
	DWORD addr_PlayerFallSM_PushTrapCam = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 64 53 56 57 33 F6 56 56 8B F9 E8 ?? ?? ?? ?? 50 56 E8 ?? ?? ?? ?? 8B D8 A1", "PlayerFallSM_PushTrapCam");
	DWORD addr_PlayerTransitionToGravitySM_PushLandingCam = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 68 56 8B F1 83 7E 78 00", "PlayerTransitionToGravitySM_PushLandingCam");
	DWORD addr_Sentient_SpawnObserverPoleCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 64 53 56 57 33 F6 56 56 8B F9 E8 ?? ?? ?? ?? 50 56 E8 ?? ?? ?? ?? 8B D8 83 C4 10", "Sentient_SpawnObserverPoleCamera");
	DWORD addr_PairedAttackCoordinatorSM_PushCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC D4 01 00 00 53 8B D9 8B 43 40", "PairedAttackCoordinatorSM_PushCamera");
	DWORD addr_PoleCamera_Init = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 8B 45 0C 83 EC 14 53 8B 5D 08", "PoleCamera_Init");

	if (addr_CameraManager_SetRenderCamera == 0 ||
		addr_CameraManager_GetActiveCamFov == 0 ||
		addr_PlayerFallSM_PushTrapCam == 0 ||
		addr_PlayerTransitionToGravitySM_PushLandingCam == 0 ||
		addr_Sentient_SpawnObserverPoleCamera == 0 ||
		addr_PairedAttackCoordinatorSM_PushCamera == 0 ||
		addr_PoleCamera_Init == 0) {
		return;
	}

	CameraManager_SetRenderCamera = HookHelper::CreateHook((void*)addr_CameraManager_SetRenderCamera, &CameraManager_SetRenderCamera_Hook);
	CameraManager_GetActiveCamFov = HookHelper::CreateHook((void*)addr_CameraManager_GetActiveCamFov, &CameraManager_GetActiveCamFov_Hook);
	PlayerFallSM_PushTrapCam = HookHelper::CreateHook((void*)addr_PlayerFallSM_PushTrapCam, &PlayerFallSM_PushTrapCam_Hook);
	PlayerTransitionToGravitySM_PushLandingCam = HookHelper::CreateHook((void*)addr_PlayerTransitionToGravitySM_PushLandingCam, &PlayerTransitionToGravitySM_PushLandingCam_Hook);
	Sentient_SpawnObserverPoleCamera = HookHelper::CreateHook((void*)addr_Sentient_SpawnObserverPoleCamera, &Sentient_SpawnObserverPoleCamera_Hook);
	PairedAttackCoordinatorSM_PushCamera = HookHelper::CreateHook((void*)addr_PairedAttackCoordinatorSM_PushCamera, &PairedAttackCoordinatorSM_PushCamera_Hook);
	PoleCamera_Init = HookHelper::CreateHook((void*)addr_PoleCamera_Init, &PoleCamera_Init_Hook);
}
