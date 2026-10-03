#pragma once

#include "../../Globals.cpp"

// =========================
// FixShadowBlur
// =========================

safetyhook::InlineHook BlurAttenuationBuffer;

static int __cdecl BlurAttenuationBuffer_Hook(unsigned int shadowAttenuationRenderTarget, float depth_Diff_For_No_Blur_Close, float depth_Diff_For_No_Blur_Far, float far_And_Close_Blend_Distance, float level_for_backcompositing, int nbrPasses, char doSkyCulling)
{
	if (g_State.currentHeight > 720)
	{
		nbrPasses = static_cast<int>(static_cast<float>(nbrPasses) * g_State.resolutionScale * g_State.resolutionScale);
	}

	return BlurAttenuationBuffer.unsafe_ccall<int>(shadowAttenuationRenderTarget, depth_Diff_For_No_Blur_Close, depth_Diff_For_No_Blur_Far, far_And_Close_Blend_Distance, level_for_backcompositing, nbrPasses, doSkyCulling);
}

static void ApplyFixShadowBlur()
{
	if (!FixShadowBlur) return;

	DWORD addr_BlurAttenuationBuffer = ScanModuleSignature(g_State.GameModule, "83 EC 44 53 56 57 83 F8 07 73 0A B8 07 00 00 00 A3", "BlurAttenuationBuffer");

	if (addr_BlurAttenuationBuffer == 0) return;

	BlurAttenuationBuffer = HookHelper::CreateHook((void*)(addr_BlurAttenuationBuffer - 0xB), &BlurAttenuationBuffer_Hook);
}
