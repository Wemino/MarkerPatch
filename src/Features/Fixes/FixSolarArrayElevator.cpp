#pragma once

#include "../../Globals.cpp"

// =========================
// FixSolarArrayElevator
// =========================

safetyhook::InlineHook EventScheduler_SendDelayedMsgToEventHandler;

static safetyhook::MidHook ElevatorFlushHook{};

static char __fastcall EventScheduler_SendDelayedMsgToEventHandler_Hook(int* thisp, int, DWORD* eventId, int pEvHandler, float delay, unsigned int msgHandle, int flags)
{
	if (eventId && eventId[0] == 0x716F8DC9)
	{
		g_State.elevatorFixArmed = true;
	}

	return EventScheduler_SendDelayedMsgToEventHandler.unsafe_thiscall<char>(thisp, eventId, pEvHandler, delay, msgHandle, flags);
}

static void OnElevatorFlush(safetyhook::Context& ctx)
{
	if (!g_State.elevatorFixArmed) return;

	if (MemoryHelper::ReadMemory<uint32_t>(ctx.esi + 0xC) == 0x808075B6)
	{
		ctx.ebp = 0x7FFFFFFF;
		g_State.elevatorFixArmed = false;
	}
}

static void ApplyFixSolarArrayElevator()
{
	if (!FixSolarArrayElevator) return;

	EventScheduler_SendDelayedMsgToEventHandler = HookHelper::CreateHook((void*)GetAddress(Addr::EventScheduler_SendDelayedMsgToEventHandler), &EventScheduler_SendDelayedMsgToEventHandler_Hook);

	ElevatorFlushHook = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::ElevatorFlushHook)), OnElevatorFlush);
}
