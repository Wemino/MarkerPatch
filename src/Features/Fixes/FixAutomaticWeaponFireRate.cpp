#pragma once
#include <cmath>
#include <algorithm>

#include "../../Globals.cpp"

// =========================
// FixAutomaticWeaponFireRate
// =========================

safetyhook::InlineHook Item_IsReadyToUse;
safetyhook::InlineHook TimeManager_HandleEvents;
safetyhook::InlineHook Item_ResetUseTimer;

static float GetConsoleFireDelay(float fireDelay, bool usesFireAnim)
{
	return (ceilf(fireDelay / TARGET_FRAME_TIME - 0.01f) + usesFireAnim) * TARGET_FRAME_TIME * 1000.0f;
}

static int __fastcall TimeManager_HandleEvents_Hook(DWORD* thisp, int, DWORD* msg)
{
	int result = TimeManager_HandleEvents.unsafe_thiscall<int>(thisp, msg);
	float frameTime = MemoryHelper::ReadMemory<float>(g_Addresses.FrameTimeSecPtr);
	g_State.frameTime += (std::min(frameTime, g_State.frameTime * 2.0f) - g_State.frameTime) * 0.2f;
	return result;
}

static bool __fastcall Item_IsReadyToUse_Hook(int thisp, int)
{
	float* fireDelayPtr = (float*)(thisp + 800);
	float originalDelay = *fireDelayPtr;
	DWORD lastShotTime = MemoryHelper::ReadMemory<DWORD>(thisp + 792);
	bool isTracked = lastShotTime == g_State.lastShotTime;

	// Only apply to automatic weapons, aim for the frame closest to the console timing
	if (originalDelay <= 0.1f && lastShotTime != 0)
	{
		float frameMs = g_State.frameTime * 1000.0f;
		bool usesFireAnim = !isTracked || g_State.usesFireAnim;
		float shotError = isTracked ? g_State.shotError : 0.0f;
		float delayMs = GetConsoleFireDelay(originalDelay, usesFireAnim) - shotError - (usesFireAnim ? frameMs : 0.0f) - frameMs * 0.5f + std::clamp(shotError, -frameMs * 0.25f, frameMs * 0.25f);
		*fireDelayPtr = std::max(delayMs, 0.0f) / 1000.0f;
	}

	bool result = Item_IsReadyToUse.unsafe_thiscall<bool>(thisp);
	*fireDelayPtr = originalDelay;

	if (result && isTracked && g_State.readyTime == 0)
	{
		g_State.readyTime = MemoryHelper::ReadMemory<DWORD>(g_Addresses.SimTimeElapsedMSecPtr);
	}

	return result;
}

static void __fastcall Item_ResetUseTimer_Hook(int thisp, int)
{
	DWORD previousShotTime = MemoryHelper::ReadMemory<DWORD>(thisp + 792);
	Item_ResetUseTimer.unsafe_thiscall<void>(thisp);
	DWORD shotTime = MemoryHelper::ReadMemory<DWORD>(thisp + 792);
	float frameMs = g_State.frameTime * 1000.0f;

	if (g_State.readyTime != 0 && shotTime - g_State.readyTime <= frameMs * 4.0f)
	{
		g_State.usesFireAnim = shotTime != g_State.readyTime;
	}

	// Keep the console timing while the trigger is held, restart it after a hitch or a new burst
	float shotError = (shotTime - previousShotTime) - GetConsoleFireDelay(MemoryHelper::ReadMemory<float>(thisp + 800), g_State.usesFireAnim) + g_State.shotError;
	bool isHeld = previousShotTime != 0 && previousShotTime == g_State.lastShotTime && fabsf(shotError) <= frameMs * 0.85f + 1.0f;

	g_State.shotError = isHeld ? shotError : 0.0f;
	g_State.lastShotTime = shotTime;
	g_State.readyTime = 0;
}

static void ApplyFixAutomaticWeaponFireRate()
{
	if (!FixAutomaticWeaponFireRate) return;

	DWORD addr_TimeManager_HandleEvents = GetAddress(Addr::TimeManager_HandleEvents);
	DWORD addr_Item_IsReadyToUse = GetAddress(Addr::Item_IsReadyToUse);
	DWORD addr_Item_ResetUseTimer = GetAddress(Addr::Item_ResetUseTimer);

	g_Addresses.FrameTimeSecPtr = GetAddress(Addr::FrameTimeSecPtr);
	g_Addresses.SimTimeElapsedMSecPtr = GetAddress(Addr::SimTimeElapsedMSecPtr);
	g_State.frameTime = TARGET_FRAME_TIME;

	TimeManager_HandleEvents = HookHelper::CreateHook((void*)addr_TimeManager_HandleEvents, &TimeManager_HandleEvents_Hook);
	Item_IsReadyToUse = HookHelper::CreateHook((void*)addr_Item_IsReadyToUse, &Item_IsReadyToUse_Hook);
	Item_ResetUseTimer = HookHelper::CreateHook((void*)addr_Item_ResetUseTimer, &Item_ResetUseTimer_Hook);
}
