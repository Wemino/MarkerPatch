#pragma once

#include "../../Globals.cpp"

// =========================
// FixStreamingBudget
// =========================

safetyhook::InlineHook TimeSlicer_GetTimeRemainingBeforeVBlank;

static uint64_t __fastcall TimeSlicer_GetTimeRemainingBeforeVBlank_Hook(uintptr_t thisp)
{
	double frameTimeMs = g_State.refreshPeriodMs;
	int presentInterval = MemoryHelper::ReadMemory<int>(g_Addresses.PresentIntervalsPtr + MemoryHelper::ReadMemory<int>(g_Addresses.PresentModePtr) * 8);

	if (presentInterval > 0 && MemoryHelper::ReadMemory<bool>(g_Addresses.FrameLimiterEnabledPtr))
	{
		frameTimeMs = MemoryHelper::ReadMemory<float>(g_Addresses.TargetFrameTimeMsPtr) * presentInterval;
	}

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	double ticksPerMs = g_State.qpcFrequency.QuadPart / 1000.0;
	double timeLeftMs = frameTimeMs * 0.9 - (now.QuadPart - *reinterpret_cast<LONGLONG*>(thisp + 0xA0)) / ticksPerMs;

	// 0 would mean no limit to the streaming code
	return static_cast<uint64_t>(std::max(timeLeftMs, 1.0) * ticksPerMs);
}

static void ApplyFixStreamingBudget()
{
	if (!FixStreamingBudget) return;

	DWORD addr_TimeSlicer_GetTimeRemainingBeforeVBlank = GetAddress(Addr::TimeSlicer_GetTimeRemainingBeforeVBlank);

	g_Addresses.FrameLimiterEnabledPtr = GetAddress(Addr::FrameLimiterEnabledPtr);
	g_Addresses.TargetFrameTimeMsPtr = GetAddress(Addr::TargetFrameTimeMsPtr);
	g_Addresses.PresentModePtr = GetAddress(Addr::PresentModePtr);
	g_Addresses.PresentIntervalsPtr = GetAddress(Addr::PresentIntervalsPtr);

	DWORD refreshRate = SystemHelper::GetCurrentDisplayFrequency();
	if (refreshRate > 1) g_State.refreshPeriodMs = 1000.0 / refreshRate;

	QueryPerformanceFrequency(&g_State.qpcFrequency);

	TimeSlicer_GetTimeRemainingBeforeVBlank = HookHelper::CreateHook((void*)addr_TimeSlicer_GetTimeRemainingBeforeVBlank, &TimeSlicer_GetTimeRemainingBeforeVBlank_Hook);
}
