#pragma once

#include "../../Globals.cpp"
static void ApplyMainLoopHook()
{
	if (!HavokPhysicsFix && !RawMouseInput && !AchievementSupport) return;

	DWORD addr_MainLoop = ScanModuleSignature(g_State.GameModule, "83 EC 20 56 57 8B 3D ?? ?? ?? ?? 6A 03 33 F6 56", "MainLoop");

	if (addr_MainLoop == 0) return;

	MainLoop = HookHelper::CreateHook((void*)addr_MainLoop, &MainLoop_Hook);
}
