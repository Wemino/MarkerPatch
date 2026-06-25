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

	DWORD addr_ShadowRes = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 E0 E0 83 EC 0C 3B 05", "ShadowRes");

	if (addr_ShadowRes == 0) return;

	SetDynamicShadowMapResolution = HookHelper::CreateHook((void*)addr_ShadowRes, &SetDynamicShadowMapResolution_Hook);
}
