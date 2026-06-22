#pragma once

#include "../../Globals.cpp"

// =========================
// FixShadowBlur
// =========================

safetyhook::InlineHook IterativeShadowBlur;

static int __cdecl IterativeShadowBlur_Hook(unsigned int a1, float a2, float a3, float a4, float a5, int passCount, char a7)
{
	if (g_State.currentHeight > 720)
	{
		passCount = static_cast<int>(static_cast<float>(passCount) * g_State.resolutionScale * g_State.resolutionScale);
	}

	return IterativeShadowBlur.unsafe_ccall<int>(a1, a2, a3, a4, a5, passCount, a7);
}
static void ApplyFixShadowBlur()
{
	if (!FixShadowBlur) return;

	DWORD addr_ShadowBlur = ScanModuleSignature(g_State.GameModule, "83 EC 44 53 56 57 83 F8 07 73 0A B8 07 00 00 00 A3", "ShadowBlur");

	if (addr_ShadowBlur == 0) return;

	IterativeShadowBlur = HookHelper::CreateHook((void*)(addr_ShadowBlur - 0xB), &IterativeShadowBlur_Hook);
}
