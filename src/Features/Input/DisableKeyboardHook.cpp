#pragma once

#include "../../Globals.cpp"

static void ApplyDisableKeyboardHook()
{
	if (!DisableKeyboardHook) return;

	DWORD addr_SetWindowsHook = ScanModuleSignature(g_State.GameModule, "83 3D ?? ?? ?? ?? 00 75 2B E8", "SetWindowsHook");

	if (addr_SetWindowsHook == 0) return;

	MemoryHelper::WriteMemory<uint8_t>(addr_SetWindowsHook, 0xC3);
	MemoryHelper::WriteMemory<uint8_t>(addr_SetWindowsHook + 0x50, 0xC3);
}