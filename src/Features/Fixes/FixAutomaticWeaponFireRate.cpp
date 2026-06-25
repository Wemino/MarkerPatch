#pragma once

#include "../../Globals.cpp"

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
