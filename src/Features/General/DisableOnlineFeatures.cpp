#pragma once

#include "../../Globals.cpp"

// =========================
// DisableOnlineFeatures
// =========================

safetyhook::InlineHook UIComponentManager_ShowScreen_Nucleus_Connecting;

static int __cdecl UIComponentManager_ShowScreen_Nucleus_Connecting_Hook(const char* title, int cancelHandler)
{
	if (strcmp(title, "$dlc_package_scanning") == 0 || strcmp(title, "$ui_nu00_connectingTitle_mc") == 0)
	{
		return 0;
	}

	return UIComponentManager_ShowScreen_Nucleus_Connecting.ccall<int>(title, cancelHandler);
}

static void ApplyDisableOnlineFeatures()
{
	if (!DisableOnlineFeatures) return;

	DWORD addr_UIComponentManager_ShowScreen_Nucleus_Connecting = ScanModuleSignature(g_State.GameModule, "83 EC 0C 83 3D ?? ?? ?? ?? ?? 74 5A", "UIComponentManager_ShowScreen_Nucleus_Connecting");
	DWORD addr_StartNucleusLogin = ScanModuleSignature(g_State.GameModule, "75 0E 8B CF E8 ?? ?? ?? ?? 5F 5E 5B 83", "StartNucleusLogin");
	DWORD addr_ShopOfflineMessage = ScanModuleSignature(g_State.GameModule, "74 25 8B 86 F8 0A 00 00", "ShopOfflineMessage");

	if (addr_UIComponentManager_ShowScreen_Nucleus_Connecting == 0 ||
		addr_StartNucleusLogin == 0 ||
		addr_ShopOfflineMessage == 0) {
		return;
	}

	UIComponentManager_ShowScreen_Nucleus_Connecting = HookHelper::CreateHook((void*)addr_UIComponentManager_ShowScreen_Nucleus_Connecting, &UIComponentManager_ShowScreen_Nucleus_Connecting_Hook);
	MemoryHelper::MakeNOP(addr_StartNucleusLogin, 2);
	MemoryHelper::MakeNOP(addr_ShopOfflineMessage, 2);
}
