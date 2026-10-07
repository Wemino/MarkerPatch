#pragma once

#include "../../Globals.cpp"

// =========================
// DynamicShadowResolution
// =========================

static safetyhook::InlineHook SetDynamicShadowMapResolution;

static unsigned int __cdecl SetDynamicShadowMapResolution_Hook(int shadowRes)
{
	if (shadowRes == 1920)
	{
		shadowRes = DynamicShadowResolution;
	}

	return SetDynamicShadowMapResolution.ccall<unsigned int>(shadowRes);
}

static void ApplyDynamicShadowResolution()
{
	if (DynamicShadowResolution <= 1920) return;

	DWORD addr_ShadowRes = GetAddress(Addr::ShadowRes);

	SetDynamicShadowMapResolution = HookHelper::CreateHook((void*)addr_ShadowRes, &SetDynamicShadowMapResolution_Hook);
}
