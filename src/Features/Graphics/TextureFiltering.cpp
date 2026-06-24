#pragma once

#include "../../Globals.cpp"

// =====================
// MaxAnisotropy
// =====================

static safetyhook::InlineHook TX_ChangeOptions;

static int __cdecl TX_ChangeOptions_Hook(int a1, int a2)
{
	int result = TX_ChangeOptions.unsafe_ccall<int>(a1, a2);

	// Get current filtering flags
	int* flags_ptr = (int*)(a2 + 16);
	int flags = *flags_ptr;
	int filtering_mode = flags & 0x30000;

	// Anisotropic Filtering
	if (MaxAnisotropy > 0)
	{
		// Upgrade point (0x0) or linear (0x20000) filtering to anisotropic
		if (filtering_mode == 0x00000 || filtering_mode == 0x20000)
		{
			*(unsigned char*)(a2 + 37) = 3;              // Anisotropic mip mode
			*(unsigned char*)(a2 + 39) = MaxAnisotropy;  // Max anisotropy level
			*flags_ptr = (flags & ~0x30000) | 0x20000;   // Ensure linear flag is set
		}
	}

	// Trilinear Filtering
	if (ForceTrilinearFiltering)
	{
		unsigned char mip_filter = *(unsigned char*)(a2 + 36);
		if (mip_filter == 1) // Point mip filtering
		{
			*(unsigned char*)(a2 + 36) = 2; // Upgrade to linear mip filtering
		}
	}

	return result;
}
static void ApplyTextureFiltering()
{
	if (MaxAnisotropy == 0 && !ForceTrilinearFiltering) return;

	DWORD addr_InitTextureSampler = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 53 8B 58 10 8B CB 8B D3 81 E1 00 00", "TX_ChangeOptions");

	if (addr_InitTextureSampler == 0) return;

	TX_ChangeOptions = HookHelper::CreateHook((void*)addr_InitTextureSampler, &TX_ChangeOptions_Hook);
}
