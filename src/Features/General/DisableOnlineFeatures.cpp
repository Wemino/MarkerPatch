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

	DWORD addr_UIComponentManager_ShowScreen_Nucleus_Connecting = GetAddress(Addr::UIComponentManager_ShowScreen_Nucleus_Connecting);
	DWORD addr_StartNucleusLogin = GetAddress(Addr::StartNucleusLogin);
	DWORD addr_ShopOfflineMessage = GetAddress(Addr::ShopOfflineMessage);

	UIComponentManager_ShowScreen_Nucleus_Connecting = HookHelper::CreateHook((void*)addr_UIComponentManager_ShowScreen_Nucleus_Connecting, &UIComponentManager_ShowScreen_Nucleus_Connecting_Hook);
	MemoryHelper::MakeNOP(addr_StartNucleusLogin, 2);
	MemoryHelper::MakeNOP(addr_ShopOfflineMessage, 2);
}
