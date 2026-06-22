#pragma once

#include "../../Globals.cpp"

// =========================
// FixBlurResolution
// =========================

safetyhook::InlineHook RenderBokehBlur;

static int __cdecl RenderBokehBlur_Hook(int a1, int a2, float cocRadius, float a4, char a5, int a6, int a7, float a8, float a9)
{
	if (g_State.currentHeight > 720)
	{
		cocRadius *= g_State.resolutionScale;
	}

	return RenderBokehBlur.unsafe_ccall<int>(a1, a2, cocRadius, a4, a5, a6, a7, a8, a9);
}
static void ApplyFixBlurResolution()
{
	if (!FixBlurResolution) return;

	DWORD addr_BlurResolution = ScanModuleSignature(g_State.GameModule, "51 E8 ?? ?? ?? FF 50 8D 4C 24 04 E8", "BlurResolution");

	if (addr_BlurResolution == 0) return;

	RenderBokehBlur = HookHelper::CreateHook((void*)(addr_BlurResolution), &RenderBokehBlur_Hook);
}
