#pragma once

#include "../../Globals.cpp"

// =========================
// FixBlurResolution
// =========================

safetyhook::InlineHook ScreenGaussianBlur_RenderImmediate;
safetyhook::InlineHook AlchemyZoomBlurShader_SetSizeAndCenterZoom;

static safetyhook::MidHook ScreenBloomBlur{};
static safetyhook::MidHook ScreenDofBlur{};
static safetyhook::MidHook ScreenGlowBlur{};
static safetyhook::MidHook ScreenDistortBlur{};

static int __cdecl ScreenGaussianBlur_RenderImmediate_Hook(int pRC, int color, float radius, float alpha, char useMipMapFastEffect, int useHalfRenderTarget, int dofparams, float minradius, float maxradius)
{
	if (g_State.currentHeight > 720)
	{
		radius *= g_State.resolutionScale;
	}

	return ScreenGaussianBlur_RenderImmediate.unsafe_ccall<int>(pRC, color, radius, alpha, useMipMapFastEffect, useHalfRenderTarget, dofparams, minradius, maxradius);
}

static int __fastcall AlchemyZoomBlurShader_SetSizeAndCenterZoom_Hook(DWORD* thisp, int, float* imageSizeXY_centerZoom)
{
	if (g_State.currentHeight > 720)
	{
		imageSizeXY_centerZoom[0] *= g_State.resolutionScale; // size X
		imageSizeXY_centerZoom[1] *= g_State.resolutionScale; // size Y
	}

	return AlchemyZoomBlurShader_SetSizeAndCenterZoom.unsafe_thiscall<int>(thisp, imageSizeXY_centerZoom);
}

static void OnScreenBloomBlur(safetyhook::Context& ctx)
{
	if (g_State.currentHeight > 720)
	{
		*reinterpret_cast<float*>(ctx.esp) *= g_State.resolutionScale; // bloom radius
		*reinterpret_cast<float*>(ctx.esp + 0x4) *= g_State.resolutionScale; // black bloom radius
	}
}

static void OnScreenDofBlur(safetyhook::Context& ctx)
{
	if (g_State.currentHeight > 720)
	{
		*reinterpret_cast<float*>(ctx.esp + 0x4) *= g_State.resolutionScale;
	}
}

static void OnSinglePassBlur(safetyhook::Context& ctx)
{
	if (g_State.currentHeight > 720)
	{
		float* radius = reinterpret_cast<float*>(ctx.esp + 0x10);
		*radius = std::min(*radius * g_State.resolutionScale, MAX_BLUR_RADIUS);
	}
}

static void ApplyFixBlurResolution()
{
	if (!FixBlurResolution) return;

	DWORD addr_ScreenGaussianBlur_RenderImmediate = ScanModuleSignature(g_State.GameModule, "51 E8 ?? ?? ?? FF 50 8D 4C 24 04 E8", "ScreenGaussianBlur_RenderImmediate");
	DWORD addr_AlchemyZoomBlurShader_SetSizeAndCenterZoom = ScanModuleSignature(g_State.GameModule, "8B 41 14 3B 81 CC 00 00 00 75 18", "AlchemyZoomBlurShader_SetSizeAndCenterZoom");
	DWORD addr_ScreenBloomBlur = ScanModuleSignature(g_State.GameModule, "D9 5C 24 04 D9 44 24 34 D9 1C 24 53 E8", "ScreenBloomBlur");
	DWORD addr_ScreenDofBlur = ScanModuleSignature(g_State.GameModule, "6A 00 50 51 D9 1C 24 56 E8", "ScreenDofBlur");
	DWORD addr_ScreenGlowBlur = ScanModuleSignature(g_State.GameModule, "D9 1C 24 52 50 57 57 E8", "ScreenGlowBlur");
	DWORD addr_ScreenDistortBlur = ScanModuleSignature(g_State.GameModule, "83 C4 04 50 53 E8 ?? ?? ?? ?? 83 C4 04 50 E8 ?? ?? ?? ?? 83 C4 24", "ScreenDistortBlur");

	if (addr_ScreenGaussianBlur_RenderImmediate == 0 ||
		addr_AlchemyZoomBlurShader_SetSizeAndCenterZoom == 0 ||
		addr_ScreenBloomBlur == 0 ||
		addr_ScreenDofBlur == 0 ||
		addr_ScreenGlowBlur == 0 ||
		addr_ScreenDistortBlur == 0) return;

	ScreenGaussianBlur_RenderImmediate = HookHelper::CreateHook((void*)(addr_ScreenGaussianBlur_RenderImmediate), &ScreenGaussianBlur_RenderImmediate_Hook);
	AlchemyZoomBlurShader_SetSizeAndCenterZoom = HookHelper::CreateHook((void*)(addr_AlchemyZoomBlurShader_SetSizeAndCenterZoom), &AlchemyZoomBlurShader_SetSizeAndCenterZoom_Hook);

	ScreenBloomBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenBloomBlur + 0xB), OnScreenBloomBlur);
	ScreenDofBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenDofBlur + 0x8), OnScreenDofBlur);
	ScreenGlowBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenGlowBlur + 0x7), OnSinglePassBlur);
	ScreenDistortBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenDistortBlur + 0xE), OnSinglePassBlur);
}
