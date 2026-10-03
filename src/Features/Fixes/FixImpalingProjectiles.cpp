#pragma once

#include "../../Globals.cpp"

// =========================
// FixImpalingProjectiles
// =========================

safetyhook::InlineHook ImpalingProjectile_ProjectileTick;

static std::unordered_map<uintptr_t, LONGLONG> g_impalingNextStep;

static void __fastcall ImpalingProjectile_ProjectileTick_Hook(uintptr_t thisp)
{
	uint32_t& accelerationFramesLeft = *reinterpret_cast<uint32_t*>(thisp + 0x584);

	if (accelerationFramesLeft == 0)
	{
		if (!g_impalingNextStep.empty()) g_impalingNextStep.erase(thisp);
		ImpalingProjectile_ProjectileTick.unsafe_thiscall<void>(thisp);
		return;
	}

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	if (!g_impalingNextStep.contains(thisp))
	{
		std::erase_if(g_impalingNextStep, [&](const auto& step) { return now.QuadPart - step.second > g_State.qpcFrequency.QuadPart; });
		g_impalingNextStep[thisp] = now.QuadPart;
	}

	// The push lasts 5 frames without gravity, keep them at 30 FPS by hiding the count in between
	if (now.QuadPart < g_impalingNextStep[thisp])
	{
		uint32_t framesLeft = accelerationFramesLeft;
		accelerationFramesLeft = 0;

		ImpalingProjectile_ProjectileTick.unsafe_thiscall<void>(thisp);

		// Unless it impaled something again during the tick
		if (accelerationFramesLeft == 0) accelerationFramesLeft = framesLeft;
		return;
	}

	g_impalingNextStep[thisp] += static_cast<LONGLONG>(TARGET_FRAME_TIME * g_State.qpcFrequency.QuadPart);
	ImpalingProjectile_ProjectileTick.unsafe_thiscall<void>(thisp);
}

static void ApplyFixImpalingProjectiles()
{
	if (!FixImpalingProjectiles) return;

	DWORD addr_ImpalingProjectile_ProjectileTick = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 44 53 56 57 8B F9 8B 9F C0 02 00 00 C1 EB 07 80 E3 01 E8", "ImpalingProjectile_ProjectileTick");

	if (addr_ImpalingProjectile_ProjectileTick == 0) return;

	QueryPerformanceFrequency(&g_State.qpcFrequency);

	ImpalingProjectile_ProjectileTick = HookHelper::CreateHook((void*)addr_ImpalingProjectile_ProjectileTick, &ImpalingProjectile_ProjectileTick_Hook);
}
