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

	DWORD addr_TimeSlicer_GetTimeRemainingBeforeVBlank = ScanModuleSignature(g_State.GameModule, "83 EC 08 A1 ?? ?? ?? ?? 53 55 56 8B F1 8B 0D ?? ?? ?? ?? 89 4C 24 0C", "TimeSlicer_GetTimeRemainingBeforeVBlank");
	DWORD addr_PresentInterval = ScanModuleSignature(g_State.GameModule, "A1 ?? ?? ?? ?? 8B 04 C5 ?? ?? ?? ?? 38 5C 24 08 75 ?? 3B C3 74 ?? 50 E8", "PresentInterval");

	if (addr_TimeSlicer_GetTimeRemainingBeforeVBlank == 0 ||
		addr_PresentInterval == 0) {
		return;
	}

	if (g_Addresses.FrameLimiterEnabledPtr == 0 || g_Addresses.TargetFrameTimeMsPtr == 0)
	{
		DWORD addr_fpsLimiter = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F8 83 EC 20 53 33 DB 56 38", "fpsLimiter");

		if (addr_fpsLimiter == 0) return;

		g_Addresses.FrameLimiterEnabledPtr = MemoryHelper::ReadMemory<int>(addr_fpsLimiter + 0xF);
		g_Addresses.TargetFrameTimeMsPtr = MemoryHelper::ReadMemory<int>(addr_fpsLimiter + 0x37);
	}

	g_Addresses.PresentModePtr = MemoryHelper::ReadMemory<int>(addr_PresentInterval + 0x1);
	g_Addresses.PresentIntervalsPtr = MemoryHelper::ReadMemory<int>(addr_PresentInterval + 0x8);

	DWORD refreshRate = SystemHelper::GetCurrentDisplayFrequency();
	if (refreshRate > 1) g_State.refreshPeriodMs = 1000.0 / refreshRate;

	QueryPerformanceFrequency(&g_State.qpcFrequency);

	TimeSlicer_GetTimeRemainingBeforeVBlank = HookHelper::CreateHook((void*)addr_TimeSlicer_GetTimeRemainingBeforeVBlank, &TimeSlicer_GetTimeRemainingBeforeVBlank_Hook);
}
