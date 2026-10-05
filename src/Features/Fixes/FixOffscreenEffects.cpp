#pragma once

#include "../../Globals.cpp"

// =========================
// FixOffscreenEffects
// =========================

safetyhook::InlineHook TFXSequencer_Simulate;

struct OffscreenEffect
{
	float hiddenTime = 0.0f; // Seconds out of view, the game's count as frames at 30 FPS
	int16_t hiddenTicks = -1; // The game's count after the last update, to see when it resets it
};

static std::unordered_map<uintptr_t, OffscreenEffect> g_offscreenEffects;

static int __fastcall TFXSequencer_Simulate_Hook(uintptr_t thisp, int, float deltaTime)
{
	int16_t& hiddenTicks = *reinterpret_cast<int16_t*>(thisp + 0x82);
	OffscreenEffect& effect = g_offscreenEffects[thisp];

	// The game sets its count when the effect is drawn (0), restarted (0) or created (28)
	if (hiddenTicks != effect.hiddenTicks)
	{
		effect.hiddenTime = hiddenTicks * TARGET_FRAME_TIME;
	}

	// It stops effects out of view for 30 frames, count them as 30 FPS frames so that stays one second
	int16_t ticks = static_cast<int16_t>(std::min(30, static_cast<int>(effect.hiddenTime * 30.0f + 0.01f)));
	hiddenTicks = ticks;

	int result = TFXSequencer_Simulate.unsafe_thiscall<int>(thisp, deltaTime);

	// Only count the time of the updates the game counts
	if (hiddenTicks == ticks + 1)
	{
		effect.hiddenTime += deltaTime;
	}

	effect.hiddenTicks = hiddenTicks;

	// Deleted on the next update
	if ((*reinterpret_cast<uint32_t*>(thisp + 0xC) & 0x200000) != 0)
	{
		g_offscreenEffects.erase(thisp);
	}

	return result;
}

static void ApplyFixOffscreenEffects()
{
	if (!FixOffscreenEffects) return;

	DWORD addr_TFXSequencer_Simulate = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 24 53 56 8B F1 89 35 ?? ?? ?? ?? 8B 46 10 F7 40 14 00 00 00 04 57 74 2F 8A 86 B0 00 00 00", "TFXSequencer_Simulate");

	if (addr_TFXSequencer_Simulate == 0) return;

	TFXSequencer_Simulate = HookHelper::CreateHook((void*)addr_TFXSequencer_Simulate, &TFXSequencer_Simulate_Hook);
}