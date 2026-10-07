#pragma once

#include "../../Globals.cpp"

// =========================
// FixMainLoopSpin
// =========================

safetyhook::InlineHook SoundProviderRWAC2_IsReadyForFrame;
safetyhook::InlineHook Time_GetCurTimeInMSec;

static bool __fastcall SoundProviderRWAC2_IsReadyForFrame_Hook(uintptr_t thisp)
{
	double& lastSystemTime = *reinterpret_cast<double*>(thisp + 0x9F0);

	if (!*reinterpret_cast<bool*>(thisp + 0x9E1))
	{
		lastSystemTime = 0.0;
		g_State.framesBeforeMix = 0;
		return true;
	}

	uintptr_t sndSystem = *reinterpret_cast<uintptr_t*>(thisp + 0x9CC);
	if (sndSystem == 0) return true;

	double systemTime = *reinterpret_cast<volatile double*>(sndSystem + 0x8);
	float systemTimerPeriod = *reinterpret_cast<float*>(sndSystem + 0xD4);

	// 6 frames per mix like the game, the spinning loop used to see each mix of a Dac thread pass on its own
	if (systemTime != lastSystemTime)
	{
		int mixes = lastSystemTime != 0.0 && systemTimerPeriod > 0.0f ? std::lround((systemTime - lastSystemTime) / systemTimerPeriod) : 1;
		g_State.framesBeforeMix = 6 + std::clamp(mixes - 1, 0, 3);
		lastSystemTime = systemTime;
	}

	if (g_State.framesBeforeMix <= 0) return false;

	g_State.framesBeforeMix--;
	return true;
}

static DWORD __cdecl Time_GetCurTimeInMSec_Hook()
{
	// The timer period is already set once for the session
	return timeGetTime();
}

static void WaitForAudioSystemTime()
{
	uintptr_t soundProvider = g_Addresses.SoundProviderPtr ? *reinterpret_cast<uintptr_t*>(g_Addresses.SoundProviderPtr) : 0;
	if (soundProvider == 0 || !*reinterpret_cast<bool*>(soundProvider + 0x9E1)) return;

	uintptr_t sndSystem = *reinterpret_cast<uintptr_t*>(soundProvider + 0x9CC);
	if (sndSystem == 0 || *reinterpret_cast<volatile double*>(sndSystem + 0x8) != *reinterpret_cast<double*>(soundProvider + 0x9F0)) return;

	LARGE_INTEGER dueTime;
	dueTime.QuadPart = -5000;

	// Still wakes up for window messages and APCs, like the SleepEx it replaces
	if (g_State.mainLoopTimer && SetWaitableTimer(g_State.mainLoopTimer, &dueTime, 0, nullptr, nullptr, FALSE))
	{
		MsgWaitForMultipleObjectsEx(1, &g_State.mainLoopTimer, 2, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
	}
	else
	{
		MsgWaitForMultipleObjectsEx(0, nullptr, 1, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
	}
}

static void ApplyFixMainLoopSpin()
{
	if (!FixMainLoopSpin) return;

	g_Addresses.SoundProviderPtr = GetAddress(Addr::SoundProviderPtr);

	timeBeginPeriod(1);
	g_State.mainLoopTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

	SoundProviderRWAC2_IsReadyForFrame = HookHelper::CreateHook((void*)GetAddress(Addr::SoundProviderRWAC2_IsReadyForFrame), &SoundProviderRWAC2_IsReadyForFrame_Hook);
	Time_GetCurTimeInMSec = HookHelper::CreateHook((void*)GetAddress(Addr::Time_GetCurTimeInMSec), &Time_GetCurTimeInMSec_Hook);
}
