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

	SetHz = HookHelper::CreateHook((void*)GetAddress(Addr::SetHz), &SetHz_Hook);
	g_Addresses.TargetFrameTimeMsPtr = GetAddress(Addr::TargetFrameTimeMsPtr);
}
