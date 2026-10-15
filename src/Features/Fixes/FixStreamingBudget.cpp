#pragma once

#include "../../Globals.cpp"

// =========================
// FixStreamingBudget
// =========================

safetyhook::InlineHook TimeSlicer_GetTimeRemainingBeforeVBlank;
safetyhook::InlineHook EntityPartSpawner_Spawn;

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

static void __fastcall EntityPartSpawner_Spawn_Hook(uintptr_t thisp, int, uintptr_t entityPart)
{
	EntityPartSpawner_Spawn.unsafe_thiscall<void>(thisp, entityPart);

	int queueSize = *reinterpret_cast<int*>(thisp + 0x28);
	if (queueSize == 0) return;

	int last = *reinterpret_cast<int*>(thisp + 0x24) + queueSize - 1;
	int maxSize = *reinterpret_cast<int*>(thisp + 0x20);
	if (last >= maxSize) last -= maxSize;

	uint32_t* scheduledPart = reinterpret_cast<uint32_t*>(*reinterpret_cast<uintptr_t*>(thisp + 0x1C) + last * 8);
	if (scheduledPart[0] == entityPart)
	{
		scheduledPart[1] = MemoryHelper::ReadMemory<DWORD>(g_Addresses.ElapsedTimeMSecPtr);
	}
}

static void ApplyFixStreamingBudget()
{
	if (!FixStreamingBudget) return;

	g_Addresses.FrameLimiterEnabledPtr = GetAddress(Addr::FrameLimiterEnabledPtr);
	g_Addresses.TargetFrameTimeMsPtr = GetAddress(Addr::TargetFrameTimeMsPtr);
	g_Addresses.PresentModePtr = GetAddress(Addr::PresentModePtr);
	g_Addresses.PresentIntervalsPtr = GetAddress(Addr::PresentIntervalsPtr);

	DWORD refreshRate = SystemHelper::GetCurrentDisplayFrequency();
	if (refreshRate > 1) g_State.refreshPeriodMs = 1000.0 / refreshRate;

	QueryPerformanceFrequency(&g_State.qpcFrequency);

	TimeSlicer_GetTimeRemainingBeforeVBlank = HookHelper::CreateHook((void*)GetAddress(Addr::TimeSlicer_GetTimeRemainingBeforeVBlank), &TimeSlicer_GetTimeRemainingBeforeVBlank_Hook);

	g_Addresses.ElapsedTimeMSecPtr = GetAddress(Addr::ElapsedTimeMSecPtr);
	EntityPartSpawner_Spawn = HookHelper::CreateHook((void*)GetAddress(Addr::EntityPartSpawner_Spawn), &EntityPartSpawner_Spawn_Hook);
}
