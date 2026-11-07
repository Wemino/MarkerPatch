#pragma once

#include "../../Globals.cpp"

// =========================
// FixStuckChaseCamera
// =========================

safetyhook::InlineHook ChaseCameraUpdater_HandleEvents;

static uintptr_t(__cdecl* Player_GetPlayerFromPlayerManager)(int) = nullptr;
static void(__thiscall* Player_UpdateChaseCamera)(uintptr_t, const DWORD*, unsigned int) = nullptr;

static uintptr_t g_chaseCameraUpdater = 0;
static DWORD g_chaseCameraUpdaterId[4] = {};

static void RestoreDefaultChaseCamera(const DWORD* updaterAttributes)
{
	uintptr_t player = Player_GetPlayerFromPlayerManager(0);
	if (player == 0) return;

	uintptr_t chaseCameraInfoRef = *reinterpret_cast<uintptr_t*>(player + 0x548);
	if (chaseCameraInfoRef == 0) return;

	uintptr_t chaseCameraInfo = chaseCameraInfoRef - 0x10;
	if (*reinterpret_cast<uintptr_t*>(chaseCameraInfo) != g_Addresses.RE4ChaseCameraInfoVtablePtr) return;

	const DWORD* defaultCameraId = reinterpret_cast<const DWORD*>(chaseCameraInfo + 0x24);
	const DWORD* currentCameraId = reinterpret_cast<const DWORD*>(chaseCameraInfo + 0x1A0);
	const DWORD* updaterCameraId = updaterAttributes + 4;

	if (!MatchId(currentCameraId, updaterCameraId[0], updaterCameraId[1], updaterCameraId[2], updaterCameraId[3])) return;
	if (MatchId(currentCameraId, defaultCameraId[0], defaultCameraId[1], defaultCameraId[2], defaultCameraId[3])) return;

	DWORD cameraId[4];
	std::memcpy(cameraId, defaultCameraId, sizeof(cameraId));

	Player_UpdateChaseCamera(player, cameraId, updaterAttributes[0]);
}

static void __fastcall ChaseCameraUpdater_HandleEvents_Hook(uintptr_t thisp, int, const DWORD* msg)
{
	const DWORD* attributes = *reinterpret_cast<const DWORD**>(thisp + 0xC);
	const DWORD* updaterId = reinterpret_cast<const DWORD*>(thisp + 0x24);

	if (attributes && msg[0] == attributes[1])
	{
		g_chaseCameraUpdater = thisp;
		std::memcpy(g_chaseCameraUpdaterId, updaterId, sizeof(g_chaseCameraUpdaterId));
	}
	else if (attributes && msg[0] == 0x64DD372F && thisp == g_chaseCameraUpdater &&
		MatchId(updaterId, g_chaseCameraUpdaterId[0], g_chaseCameraUpdaterId[1], g_chaseCameraUpdaterId[2], g_chaseCameraUpdaterId[3]))
	{
		g_chaseCameraUpdater = 0;
		RestoreDefaultChaseCamera(attributes);
	}

	ChaseCameraUpdater_HandleEvents.unsafe_thiscall<void>(thisp, msg);
}

static void ApplyFixStuckChaseCamera()
{
	if (!FixStuckChaseCamera) return;

	g_Addresses.RE4ChaseCameraInfoVtablePtr = GetAddress(Addr::RE4ChaseCameraInfoVtablePtr);

	Player_GetPlayerFromPlayerManager = reinterpret_cast<decltype(Player_GetPlayerFromPlayerManager)>(GetAddress(Addr::Player_GetPlayerFromPlayerManager));
	Player_UpdateChaseCamera = reinterpret_cast<decltype(Player_UpdateChaseCamera)>(GetAddress(Addr::Player_UpdateChaseCamera));

	ChaseCameraUpdater_HandleEvents = HookHelper::CreateHook((void*)GetAddress(Addr::ChaseCameraUpdater_HandleEvents), &ChaseCameraUpdater_HandleEvents_Hook);
}
