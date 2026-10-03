#pragma once

#include "../../Globals.cpp"

// =========================
// FixFrameLimiter
// =========================

safetyhook::InlineHook FrameLimiter;

static void __cdecl FrameLimiter_Hook(int interval)
{
	float frameTimeMs = MemoryHelper::ReadMemory<float>(g_Addresses.TargetFrameTimeMsPtr) * interval;

	if (!MemoryHelper::ReadMemory<bool>(g_Addresses.FrameLimiterEnabledPtr) || frameTimeMs <= 0.0f)
	{
		g_State.nextFrameCounter = 0;
		return;
	}

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	LONGLONG frameTicks = static_cast<LONGLONG>(frameTimeMs * static_cast<double>(g_State.qpcFrequency.QuadPart) / 1000.0);

	if (now.QuadPart - g_State.nextFrameCounter > frameTicks)
	{
		g_State.nextFrameCounter = now.QuadPart;
	}

	LONGLONG wakeCounter = g_State.nextFrameCounter - static_cast<LONGLONG>(g_State.frameLimiterSpinMs * g_State.qpcFrequency.QuadPart / 1000.0);

	// Sleep on the high resolution timer until shortly before the frame, the spin follows how late it wakes up
	if (g_State.frameLimiterTimer && wakeCounter > now.QuadPart)
	{
		LARGE_INTEGER dueTime;
		dueTime.QuadPart = -(wakeCounter - now.QuadPart) * 10000000 / g_State.qpcFrequency.QuadPart;

		if (SetWaitableTimer(g_State.frameLimiterTimer, &dueTime, 0, nullptr, nullptr, FALSE))
		{
			WaitForSingleObject(g_State.frameLimiterTimer, INFINITE);
			QueryPerformanceCounter(&now);

			double lateMs = (now.QuadPart - wakeCounter) * 1000.0 / g_State.qpcFrequency.QuadPart;
			g_State.frameLimiterSpinMs = lateMs + 0.1 > g_State.frameLimiterSpinMs ? std::min(lateMs + 0.1, 2.0) : std::max(g_State.frameLimiterSpinMs - 0.002, 0.25);
		}
	}

	while (now.QuadPart < g_State.nextFrameCounter)
	{
		if (!g_State.frameLimiterTimer && g_State.nextFrameCounter - now.QuadPart > g_State.qpcFrequency.QuadPart / 500)
		{
			Sleep(1);
		}
		else
		{
			YieldProcessor();
		}

		QueryPerformanceCounter(&now);
	}

	g_State.nextFrameCounter += frameTicks;
}

static void ApplyFixFrameLimiter()
{
	if (!FixFrameLimiter) return;

	DWORD addr_fpsLimiter = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F8 83 EC 20 53 33 DB 56 38", "fpsLimiter");

	if (addr_fpsLimiter == 0) return;

	g_Addresses.FrameLimiterEnabledPtr = MemoryHelper::ReadMemory<int>(addr_fpsLimiter + 0xF);
	g_Addresses.TargetFrameTimeMsPtr = MemoryHelper::ReadMemory<int>(addr_fpsLimiter + 0x37);

	QueryPerformanceFrequency(&g_State.qpcFrequency);
	timeBeginPeriod(1);

	g_State.frameLimiterTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

	FrameLimiter = HookHelper::CreateHook((void*)addr_fpsLimiter, &FrameLimiter_Hook);
}
