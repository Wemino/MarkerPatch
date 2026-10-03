#pragma once

#include "../../Globals.cpp"

// =========================
// FixAudioSyncStall
// =========================

safetyhook::InlineHook System_IsCommandComplete;
safetyhook::InlineHook Dac_GetSamplesToMix;

static thread_local bool t_isPollingCommand = false;
static thread_local unsigned int t_polledTimeStamp = 0;

static bool __fastcall System_IsCommandComplete_Hook(uintptr_t thisp, int, unsigned int timeStamp)
{
	bool isComplete = System_IsCommandComplete.unsafe_thiscall<bool>(thisp, timeStamp);
	bool isWaiting = t_isPollingCommand && t_polledTimeStamp == timeStamp;

	t_isPollingCommand = !isComplete;
	t_polledTimeStamp = timeStamp;

	// The Dac thread only runs commands when it mixes, let it mix one more chunk ahead now
	if (isWaiting && !isComplete && g_State.maxExtraMixChunks > 0)
	{
		int chunks = g_State.extraMixChunks;
		while (chunks < g_State.maxExtraMixChunks && !g_State.extraMixChunks.compare_exchange_weak(chunks, chunks + 1)) {}

		g_State.lastCommandWaitTime = timeGetTime();
		SetEvent(g_State.dacWakeEvent);
	}

	return isComplete;
}

static int __fastcall Dac_GetSamplesToMix_Hook(uintptr_t thisp)
{
	float& latencyMs = *reinterpret_cast<float*>(thisp + 0x5C);
	float sampleRate = *reinterpret_cast<float*>(thisp + 0x38);

	// No more than a quarter of the DirectSound buffer the latency leaves free
	if (g_State.maxExtraMixChunks < 0 && sampleRate > 0.0f)
	{
		int freeSamples = *reinterpret_cast<int*>(thisp + 0x68) - static_cast<int>(latencyMs * sampleRate / 1000.0f);
		g_State.maxExtraMixChunks = std::clamp(freeSamples / 4 / 256, 0, 8);
	}

	int chunks = g_State.extraMixChunks;
	if (chunks <= 0 || sampleRate <= 0.0f) return Dac_GetSamplesToMix.unsafe_thiscall<int>(thisp);

	if (timeGetTime() - g_State.lastCommandWaitTime > 200)
	{
		g_State.extraMixChunks.compare_exchange_strong(chunks, chunks - 1);
	}

	float originalLatencyMs = latencyMs;
	latencyMs += chunks * 256 * 1000.0f / sampleRate;
	int result = Dac_GetSamplesToMix.unsafe_thiscall<int>(thisp);
	latencyMs = originalLatencyMs;

	return result;
}

static void __stdcall DacThread_Sleep_Hook(DWORD dwMilliseconds)
{
	if (WaitForSingleObject(g_State.dacWakeEvent, dwMilliseconds) != WAIT_OBJECT_0) return;

	// The extra pass adds 10 ms to the thread's next wake up time, take them back
	*reinterpret_cast<DWORD*>(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) + 0x24) -= 10;
}

static void ApplyFixAudioSyncStall()
{
	if (!FixAudioSyncStall) return;

	DWORD addr_System_IsCommandComplete = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 3B 81 00 01 00 00 1B C0 F7 D8 C2 04 00", "System_IsCommandComplete");
	DWORD addr_Dac_GetSamplesToMix = ScanModuleSignature(g_State.GameModule, "83 EC 10 83 3D ?? ?? ?? ?? 00 56 8B F1 75 07 33 C0 5E 83 C4 10 C3 80 BE 9F 00 00 00 00", "Dac_GetSamplesToMix");
	DWORD addr_DacThread_NextWakeTime = ScanModuleSignature(g_State.GameModule, "8B 7C 24 1C 83 C7 0A 89 7C 24 1C FF 15 ?? ?? ?? ?? 2B F8", "DacThread_NextWakeTime");
	DWORD addr_DacThread_Sleep = ScanModuleSignature(g_State.GameModule, "89 15 ?? ?? ?? ?? 57 FF 15 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00", "DacThread_Sleep");

	if (addr_System_IsCommandComplete == 0 ||
		addr_Dac_GetSamplesToMix == 0 ||
		addr_DacThread_NextWakeTime == 0 ||
		addr_DacThread_Sleep == 0) {
		return;
	}

	g_State.dacWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!g_State.dacWakeEvent) return;

	// call ds:Sleep is 6 bytes
	MemoryHelper::MakeCALL(addr_DacThread_Sleep + 0x7, reinterpret_cast<uintptr_t>(&DacThread_Sleep_Hook));
	MemoryHelper::MakeNOP(addr_DacThread_Sleep + 0xC, 1);

	Dac_GetSamplesToMix = HookHelper::CreateHook((void*)addr_Dac_GetSamplesToMix, &Dac_GetSamplesToMix_Hook);
	System_IsCommandComplete = HookHelper::CreateHook((void*)addr_System_IsCommandComplete, &System_IsCommandComplete_Hook);
}
