#pragma once

#include "../../Globals.cpp"

// =========================
// FixSolarArrayElevator
// =========================

safetyhook::InlineHook Timeline_ScheduleEvent;

static char __fastcall Timeline_ScheduleEvent_Hook(int* thisp, int, DWORD* a2, int a3, float a4, unsigned int a5, int a6)
{
	if (a2 && a2[0] == 0x716F8DC9)
	{
		g_State.elevatorFixArmed = true;
	}

	return Timeline_ScheduleEvent.unsafe_thiscall<char>(thisp, a2, a3, a4, a5, a6);
}
static void ApplyFixSolarArrayElevator()
{
	if (!FixSolarArrayElevator) return;

	DWORD addr_Timeline_ScheduleEvent = ScanModuleSignature(g_State.GameModule, "0F 57 C0 83 EC 24 0F 2F 44 24 30", "Timeline_ScheduleEvent");
	DWORD addr_ElevatorFlushHook = ScanModuleSignature(g_State.GameModule, "85 C0 75 4F E8", "ElevatorFlushHook");

	if (addr_Timeline_ScheduleEvent == 0 ||
		addr_ElevatorFlushHook == 0) {
		return;
	}

	Timeline_ScheduleEvent = HookHelper::CreateHook((void*)addr_Timeline_ScheduleEvent, &Timeline_ScheduleEvent_Hook);

	static SafetyHookMid ElevatorFlushHook{};
	ElevatorFlushHook = safetyhook::create_mid(reinterpret_cast<void*>(addr_ElevatorFlushHook),
		[](safetyhook::Context& ctx)
		{
			if (!g_State.elevatorFixArmed) return;

			if (MemoryHelper::ReadMemory<uint32_t>(ctx.esi + 0xC) == 0x808075B6)
			{
				ctx.ebp = 0x7FFFFFFF;
				g_State.elevatorFixArmed = false;
			}
		}
	);
}
