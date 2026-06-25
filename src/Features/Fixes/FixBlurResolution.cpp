#pragma once

#include "../../Globals.cpp"

// =========================
// FixBlurResolution
// =========================

safetyhook::InlineHook RenderBokehBlur;
safetyhook::InlineHook ZoomBlurShader_SetSizeAndCenterZoom;

static int __cdecl RenderBokehBlur_Hook(int a1, int a2, float radius, float a4, char a5, int a6, int a7, float a8, float a9)
{
	if (g_State.currentHeight > 720)
	{
		radius *= g_State.resolutionScale;
	}

	return RenderBokehBlur.unsafe_ccall<int>(a1, a2, radius, a4, a5, a6, a7, a8, a9);
}

static int __fastcall ZoomBlurShader_SetSizeAndCenterZoom_Hook(DWORD* thisp, int, float* a2)
{
	if (g_State.currentHeight > 720)
	{
		a2[0] *= g_State.resolutionScale; // size X
		a2[1] *= g_State.resolutionScale; // size Y
	}

	return ZoomBlurShader_SetSizeAndCenterZoom.unsafe_thiscall<int>(thisp, a2);
}

static void ApplyFixBlurResolution()
{
	if (!FixBlurResolution) return;

	DWORD addr_BlurResolution = ScanModuleSignature(g_State.GameModule, "51 E8 ?? ?? ?? FF 50 8D 4C 24 04 E8", "BlurResolution");
	DWORD addr_SetSizeAndCenterZoom = ScanModuleSignature(g_State.GameModule, "8B 41 14 3B 81 CC 00 00 00 75 18", "SetSizeAndCenterZoom");

	if (addr_BlurResolution == 0 ||
		addr_SetSizeAndCenterZoom == 0) return;

	RenderBokehBlur = HookHelper::CreateHook((void*)(addr_BlurResolution), &RenderBokehBlur_Hook);
	ZoomBlurShader_SetSizeAndCenterZoom = HookHelper::CreateHook((void*)(addr_SetSizeAndCenterZoom), &ZoomBlurShader_SetSizeAndCenterZoom_Hook);
}
