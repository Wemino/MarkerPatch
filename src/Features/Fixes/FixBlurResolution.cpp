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

	DWORD addr_ScreenGaussianBlur_RenderImmediate = GetAddress(Addr::ScreenGaussianBlur_RenderImmediate);
	DWORD addr_AlchemyZoomBlurShader_SetSizeAndCenterZoom = GetAddress(Addr::AlchemyZoomBlurShader_SetSizeAndCenterZoom);
	DWORD addr_ScreenBloomBlur = GetAddress(Addr::ScreenBloomBlur);
	DWORD addr_ScreenDofBlur = GetAddress(Addr::ScreenDofBlur);
	DWORD addr_ScreenGlowBlur = GetAddress(Addr::ScreenGlowBlur);
	DWORD addr_ScreenDistortBlur = GetAddress(Addr::ScreenDistortBlur);

	ScreenGaussianBlur_RenderImmediate = HookHelper::CreateHook((void*)(addr_ScreenGaussianBlur_RenderImmediate), &ScreenGaussianBlur_RenderImmediate_Hook);
	AlchemyZoomBlurShader_SetSizeAndCenterZoom = HookHelper::CreateHook((void*)(addr_AlchemyZoomBlurShader_SetSizeAndCenterZoom), &AlchemyZoomBlurShader_SetSizeAndCenterZoom_Hook);

	ScreenBloomBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenBloomBlur + 0xB), OnScreenBloomBlur);
	ScreenDofBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenDofBlur + 0x8), OnScreenDofBlur);
	ScreenGlowBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenGlowBlur + 0x7), OnSinglePassBlur);
	ScreenDistortBlur = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenDistortBlur + 0xE), OnSinglePassBlur);
}
