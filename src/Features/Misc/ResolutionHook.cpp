#pragma once

#include "../../Globals.cpp"

// ==================
// Resolution Util
// ==================

safetyhook::InlineHook SetResolution;
safetyhook::InlineHook UpdateDisplaySettings;

static __int16 __cdecl SetResolution_Hook(__int16 width, __int16 height)
{
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
	if (!FixBlurResolution && !FixShadowBlur && !VSyncRefreshRateFix) return;

	DWORD addr_UpdateDisplaySettings = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 8B 4C 24 08 B2 01 66 39 05", "UpdateDisplaySettings");
	DWORD addr_SetResolution = ScanModuleSignature(g_State.GameModule, "66 8B 44 24 04 66 8B 4C 24 08 66 A3", "SetResolution");

	if (addr_UpdateDisplaySettings == 0 ||
		addr_SetResolution == 0) {
		return;
	}

	UpdateDisplaySettings = HookHelper::CreateHook((void*)addr_UpdateDisplaySettings, &UpdateDisplaySettings_Hook);
	SetResolution = HookHelper::CreateHook((void*)addr_SetResolution, &SetResolution_Hook);
}
