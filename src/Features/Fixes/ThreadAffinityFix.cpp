#pragma once

#include "../../Globals.cpp"

// =========================
// ThreadAffinityFix
// =========================

// add esp, 8 in place of the SetThreadAffinityMask call, its two arguments are already pushed
static constexpr uint8_t SKIP_SET_THREAD_AFFINITY_MASK[] = { 0x83, 0xC4, 0x08, 0x90, 0x90, 0x90 };

// No preferred core, the thread library then spreads the threads over the cores
static constexpr int ANY_PROCESSOR = -1;

static void ApplyThreadAffinityFix()
{
	if (!ThreadAffinityFix) return;

	DWORD addr_PresentationThreadAffinity = ScanModuleSignature(g_State.GameModule, "8A 0D ?? ?? ?? ?? B8 01 00 00 00 D3 E0 56 50 FF 15 ?? ?? ?? ?? 50 FF 15", "PresentationThreadAffinity");
	DWORD addr_MainThreadAffinity = ScanModuleSignature(g_State.GameModule, "8A 0D ?? ?? ?? ?? D3 E3 83 C4 04 53 FF 15 ?? ?? ?? ?? 50 FF 15", "MainThreadAffinity");

	if (addr_PresentationThreadAffinity == 0 ||
		addr_MainThreadAffinity == 0) {
		return;
	}

	// The core of the render and main threads is never set, both are locked to the first core
	MemoryHelper::WriteMemory<int>(MemoryHelper::ReadMemory<int>(addr_PresentationThreadAffinity + 0x2), ANY_PROCESSOR);
	MemoryHelper::WriteMemory<int>(MemoryHelper::ReadMemory<int>(addr_MainThreadAffinity + 0x2), ANY_PROCESSOR);

	MemoryHelper::WriteMemoryRaw(addr_PresentationThreadAffinity + 0x16, SKIP_SET_THREAD_AFFINITY_MASK, sizeof(SKIP_SET_THREAD_AFFINITY_MASK));
	MemoryHelper::WriteMemoryRaw(addr_MainThreadAffinity + 0x13, SKIP_SET_THREAD_AFFINITY_MASK, sizeof(SKIP_SET_THREAD_AFFINITY_MASK));
}
