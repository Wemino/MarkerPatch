#pragma once

#include "../../Globals.cpp"

// ======================
// AutoResolution
// ======================

safetyhook::InlineHook GetConfigInt;

static int __cdecl GetConfigInt_Hook(const char* Src, int ArgList)
{
	if (strcmp(Src, "Window.Width") == 0)
	{
		ArgList = g_State.screenWidth;
	}
	if (strcmp(Src, "Window.Height") == 0)
	{
		ArgList = g_State.screenHeight;
	}

	return GetConfigInt.ccall<int>(Src, ArgList);
}

static void ApplyAutoResolution()
{
	if (!AutoResolution) return;

	DWORD addr_GetConfigInt = ScanModuleSignature(g_State.GameModule, "68 00 04 00 00 D9 1D ?? ?? ?? ?? 68", "GetConfigInt");
	addr_GetConfigInt = MemoryHelper::ResolveRelativeAddress(addr_GetConfigInt, 0x11);

	if (addr_GetConfigInt == 0) return;

	GetConfigInt = HookHelper::CreateHook((void*)addr_GetConfigInt, &GetConfigInt_Hook);
}
