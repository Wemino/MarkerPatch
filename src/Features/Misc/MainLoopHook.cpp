#pragma once

#include "../../Globals.cpp"

safetyhook::InlineHook MainLoop;

static int __cdecl MainLoop_Hook()
{
	if (PreloadStreamedTextures)
	{
		InstallTexturePreload();
	}

	int result = MainLoop.unsafe_ccall<int>();

	// 1 when the frame waits for the audio, the game calls this again right away
	if (result == 1 && FixMainLoopSpin)
	{
		WaitForAudioSystemTime();
	}

	if (result != 0) return result;

	if (HavokPhysicsFix)
	{
		RestoreSimulatedBodies();
	}

	if (RawMouseInput)
	{
		g_State.frameRawX = g_State.rawMouseDeltaX.exchange(0);
		g_State.frameRawY = g_State.rawMouseDeltaY.exchange(0);
		ControllerHelper::GetProcessedGyroDelta(g_State.frameGyroYaw, g_State.frameGyroPitch);
		g_State.mouseAimData = 0;
	}

	if (AchievementSupport)
	{
		AchievementOverlay::Update(GetD3D9Device());
	}

	if (PreloadStreamedTextures)
	{
		PreloadQueuedTextures();
	}

	return result;
}

static void ApplyMainLoopHook()
{
	if (!HavokPhysicsFix && !RawMouseInput && !AchievementSupport && !FixMainLoopSpin && !PreloadStreamedTextures) return;

	DWORD addr_MainLoop = ScanModuleSignature(g_State.GameModule, "83 EC 20 56 57 8B 3D ?? ?? ?? ?? 6A 03 33 F6 56", "MainLoop");

	if (addr_MainLoop == 0) return;

	MainLoop = HookHelper::CreateHook((void*)addr_MainLoop, &MainLoop_Hook);
}
