#pragma once

#include "../../Globals.cpp"

// =========================
// FixGameClock
// =========================

static safetyhook::MidHook GetCurTimeInMSec{};

static void OnGetCurTimeInMSec(safetyhook::Context& ctx)
{
	DWORD lastTimeMSec = *reinterpret_cast<DWORD*>(ctx.esi + 0x10);
	if (timeGetTime() != lastTimeMSec) return;

	LARGE_INTEGER start;
	LARGE_INTEGER now;
	QueryPerformanceCounter(&start);
	now = start;

	// The timer ticks every millisecond, give up if it doesn't
	while (timeGetTime() == lastTimeMSec && now.QuadPart - start.QuadPart <= g_State.qpcFrequency.QuadPart / 500)
	{
		YieldProcessor();
		QueryPerformanceCounter(&now);
	}
}

static void ApplyFixGameClock()
{
	if (!FixGameClock) return;

	DWORD addr_GetCurTimeInMSec = GetAddress(Addr::GetCurTimeInMSec);

	QueryPerformanceFrequency(&g_State.qpcFrequency);
	timeBeginPeriod(1);

	GetCurTimeInMSec = safetyhook::create_mid(reinterpret_cast<void*>(addr_GetCurTimeInMSec), OnGetCurTimeInMSec);
}
