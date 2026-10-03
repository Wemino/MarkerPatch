#pragma once

#include "../../Globals.cpp"

// =====================
// MaxAnisotropy
// =====================

static safetyhook::InlineHook TX_ChangeOptions_d3d;

static int __cdecl TX_ChangeOptions_d3d_Hook(int ref, int info)
{
	int result = TX_ChangeOptions_d3d.unsafe_ccall<int>(ref, info);

	// Get current filtering flags, the mode is trilinear (0x0), point (0x10000) or anisotropic 4x (0x20000)
	int flags = *(int*)(info + 16);
	int filtering_mode = flags & 0x30000;

	// Upgrade the trilinear and anisotropic textures
	if (filtering_mode == 0x00000 || filtering_mode == 0x20000)
	{
		*(unsigned char*)(info + 37) = 3;              // Anisotropic min filter
		*(unsigned char*)(info + 39) = MaxAnisotropy;  // Max anisotropy level
	}

	return result;
}

static void ApplyTextureFiltering()
{
	if (MaxAnisotropy == 0) return;

	DWORD addr_TX_ChangeOptions_d3d = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 53 8B 58 10 8B CB 8B D3 81 E1 00 00", "TX_ChangeOptions_d3d");

	if (addr_TX_ChangeOptions_d3d == 0) return;

	TX_ChangeOptions_d3d = HookHelper::CreateHook((void*)addr_TX_ChangeOptions_d3d, &TX_ChangeOptions_d3d_Hook);
}
