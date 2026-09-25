#pragma once
#include <cmath>
#include <algorithm>

#include "../../Globals.cpp"

// =========================
// FixAutomaticWeaponFireRate
// =========================

safetyhook::InlineHook CheckFireCooldown;
safetyhook::InlineHook UpdateEngineTimer;
safetyhook::InlineHook ResetUseTimer;

static float GetConsoleFireDelay(float fireDelay, bool usesFireAnim)
{
	return (ceilf(fireDelay / TARGET_FRAME_TIME - 0.01f) + usesFireAnim) * TARGET_FRAME_TIME * 1000.0f;
}

static int __fastcall UpdateEngineTimer_Hook(DWORD* thisp, int, DWORD* a2)
{
	int result = UpdateEngineTimer.unsafe_thiscall<int>(thisp, a2);
	float frameTime = MemoryHelper::ReadMemory<float>(g_Addresses.EngineFrameTimePtr);
	g_State.frameTime += (std::min(frameTime, g_State.frameTime * 2.0f) - g_State.frameTime) * 0.2f;
	return result;
}

static bool __fastcall CheckFireCooldown_Hook(int thisp, int)
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

	bool result = CheckFireCooldown.unsafe_thiscall<bool>(thisp);
	*fireDelayPtr = originalDelay;

	if (result && isTracked && g_State.readyTime == 0)
	{
		g_State.readyTime = MemoryHelper::ReadMemory<DWORD>(g_Addresses.SimTimeMsPtr);
	}

	return result;
}

static void __fastcall ResetUseTimer_Hook(int thisp, int)
{
	DWORD previousShotTime = MemoryHelper::ReadMemory<DWORD>(thisp + 792);
	ResetUseTimer.unsafe_thiscall<void>(thisp);
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

	DWORD addr_UpdateEngineTimer = ScanModuleSignature(g_State.GameModule, "83 EC 10 55 56 57 8B F1 E8", "UpdateEngineTimer");
	DWORD addr_CheckFireCooldown = ScanModuleSignature(g_State.GameModule, "51 8B 81 18 03 00 00 D9 05", "CheckFireCooldown");
	DWORD addr_ResetUseTimer = ScanModuleSignature(g_State.GameModule, "A1 ?? ?? ?? ?? 89 81 18 03 00 00 C3", "ResetUseTimer");

	if (addr_UpdateEngineTimer == 0 ||
		addr_CheckFireCooldown == 0 ||
		addr_ResetUseTimer == 0) {
		return;
	}

	g_Addresses.EngineFrameTimePtr = MemoryHelper::ReadMemory<int>(addr_UpdateEngineTimer + 0x265);
	g_Addresses.SimTimeMsPtr = MemoryHelper::ReadMemory<int>(addr_ResetUseTimer + 0x1);
	g_State.frameTime = TARGET_FRAME_TIME;

	UpdateEngineTimer = HookHelper::CreateHook((void*)addr_UpdateEngineTimer, &UpdateEngineTimer_Hook);
	CheckFireCooldown = HookHelper::CreateHook((void*)addr_CheckFireCooldown, &CheckFireCooldown_Hook);
	ResetUseTimer = HookHelper::CreateHook((void*)addr_ResetUseTimer, &ResetUseTimer_Hook);
}
