#pragma once

#include "../../Globals.cpp"

// ==================
// Resolution Util
// ==================

safetyhook::InlineHook SetResolution;
safetyhook::InlineHook UpdateDisplaySettings;

static __int16 __cdecl SetResolution_Hook(__int16 width, __int16 height)
{
	// Supersampling renders at a multiple of the configured resolution
	if (g_State.isSupersampling)
	{
		width = static_cast<__int16>(ToRenderSize(width));
		height = static_cast<__int16>(ToRenderSize(height));
	}

	g_State.currentHeight = height;
	g_State.resolutionScale = static_cast<float>(height) / 720.0f;
	return SetResolution.ccall<__int16>(width, height);
}

static __int16 __cdecl UpdateDisplaySettings_Hook(__int16 width, __int16 height, __int16 hz, char a4, char a5)
{
	g_State.currentHeight = height;
	g_State.resolutionScale = static_cast<float>(height) / 720.0f;

	if (VSyncRefreshRateFix)
	{
		MemoryHelper::WriteMemory<float>(g_Addresses.TargetFrameTimeMsPtr, CalculateFpsConstant(hz));
	}

	return UpdateDisplaySettings.ccall<__int16>(width, height, hz, a4, a5);
}

static void ApplyResolutionHook()
{
	if (!FixBlurResolution && !FixShadowBlur && !VSyncRefreshRateFix && !g_State.isSupersampling) return;

	UpdateDisplaySettings = HookHelper::CreateHook((void*)GetAddress(Addr::UpdateDisplaySettings), &UpdateDisplaySettings_Hook);
	SetResolution = HookHelper::CreateHook((void*)GetAddress(Addr::SetResolution), &SetResolution_Hook);
}
