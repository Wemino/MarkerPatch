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

	DWORD addr_SetHz = GetAddress(Addr::SetHz);

	SetHz = HookHelper::CreateHook((void*)addr_SetHz, &SetHz_Hook);
	g_Addresses.TargetFrameTimeMsPtr = GetAddress(Addr::TargetFrameTimeMsPtr);
}
