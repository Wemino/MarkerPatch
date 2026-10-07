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

	UIComponentManager_ShowScreen_Nucleus_Connecting = HookHelper::CreateHook((void*)GetAddress(Addr::UIComponentManager_ShowScreen_Nucleus_Connecting), &UIComponentManager_ShowScreen_Nucleus_Connecting_Hook);
	MemoryHelper::MakeNOP(GetAddress(Addr::StartNucleusLogin), 2);
	MemoryHelper::MakeNOP(GetAddress(Addr::ShopOfflineMessage), 2);
}
