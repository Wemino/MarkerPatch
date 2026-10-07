#pragma once

#include "../../Globals.cpp"

static void ApplyDisableKeyboardHook()
{
	if (!DisableKeyboardHook) return;

	DWORD addr_SetWindowsHook = GetAddress(Addr::SetWindowsHook);

	MemoryHelper::WriteMemory<uint8_t>(addr_SetWindowsHook, 0xC3);
	MemoryHelper::WriteMemory<uint8_t>(addr_SetWindowsHook + 0x50, 0xC3);
}