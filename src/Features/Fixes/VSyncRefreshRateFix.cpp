#pragma once

#include "../../Globals.cpp"

// =========================
// VSyncRefreshRateFix
// =========================

safetyhook::InlineHook SetHz;

static __int16 __cdecl SetHz_Hook(__int16 hz)
{
	MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	return SetHz.ccall<__int16>(hz);
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
