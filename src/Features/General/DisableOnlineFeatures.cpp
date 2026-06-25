#pragma once

#include "../../Globals.cpp"

// =========================
// DisableOnlineFeatures
// =========================

safetyhook::InlineHook DisplayUIPopup;

static int __cdecl DisplayUIPopup_Hook(const char* Src, int a2)
{
	if (strcmp(Src, "$dlc_package_scanning") == 0 || strcmp(Src, "$ui_nu00_connectingTitle_mc") == 0)
	{
		return 0;
	}

	return DisplayUIPopup.ccall<int>(Src, a2);
}

static void ApplyDisableOnlineFeatures()
{
	if (!DisableOnlineFeatures) return;

	DWORD addr_DisplayUIPopup = ScanModuleSignature(g_State.GameModule, "83 EC 0C 83 3D ?? ?? ?? ?? ?? 74 5A", "DisplayUIPopup");
	DWORD addr_StartAuth = ScanModuleSignature(g_State.GameModule, "75 0E 8B CF E8 ?? ?? ?? ?? 5F 5E 5B 83", "StartAuth");
	DWORD addr_ShopOfflineMessage = ScanModuleSignature(g_State.GameModule, "74 25 8B 86 F8 0A 00 00", "ShopOfflineMessage");

	if (addr_DisplayUIPopup == 0 ||
		addr_StartAuth == 0 ||
		addr_ShopOfflineMessage == 0) {
		return;
	}

	DisplayUIPopup = HookHelper::CreateHook((void*)addr_DisplayUIPopup, &DisplayUIPopup_Hook);
	MemoryHelper::MakeNOP(addr_StartAuth, 2);
	MemoryHelper::MakeNOP(addr_ShopOfflineMessage, 2);
}
