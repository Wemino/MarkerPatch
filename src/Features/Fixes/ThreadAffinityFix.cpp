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

	DWORD addr_PresentationThreadAffinity = GetAddress(Addr::PresentationThreadAffinity);
	DWORD addr_MainThreadAffinity = GetAddress(Addr::MainThreadAffinity);

	// The core of the render and main threads is never set, both are locked to the first core
	MemoryHelper::WriteMemory<int>(GetAddress(Addr::PresentationThreadCore), ANY_PROCESSOR);
	MemoryHelper::WriteMemory<int>(GetAddress(Addr::MainThreadCore), ANY_PROCESSOR);

	MemoryHelper::WriteMemoryRaw(addr_PresentationThreadAffinity + 0x16, SKIP_SET_THREAD_AFFINITY_MASK, sizeof(SKIP_SET_THREAD_AFFINITY_MASK));
	MemoryHelper::WriteMemoryRaw(addr_MainThreadAffinity + 0x13, SKIP_SET_THREAD_AFFINITY_MASK, sizeof(SKIP_SET_THREAD_AFFINITY_MASK));
}
