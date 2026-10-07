#pragma once

#include "../../Globals.cpp"

// =========================
// FixDifficultyRewards
// =========================

safetyhook::InlineHook UIOptions_PersistableRestore;

static bool __fastcall UIOptions_PersistableRestore_Hook(DWORD* thisPtr, int, int storageKey, DWORD* pData, int nBytes, int eventId)
{
	if (pData && (thisPtr[81] & 0x40))
	{
		// if new game+
		if (MemoryHelper::ReadMemory<DWORD>(g_Addresses.UIFrontendManagerPtr) && MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.UIFrontendManagerPtr) + 0x96C) == 3)
		{
			// write the update flag
			int ng_plus_diff = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.UIFrontendManagerPtr) + 0x978);
			MemoryHelper::WriteMemory(g_Addresses.OptionsDifficultyPtr, ng_plus_diff, false);
			MemoryHelper::WriteMemory(g_Addresses.OptionsDifficultyPtr + 0x4, ng_plus_diff, false);
		}
		// write the lowest difficulty flag from the save file (or not initialized, used to check for achievements)
		MemoryHelper::WriteMemory(g_Addresses.OptionsDifficultyPtr + 0x4, pData[14], false);
	}

	return UIOptions_PersistableRestore.thiscall<bool>(thisPtr, storageKey, pData, nBytes, eventId);
}

static void ApplyFixDifficultyRewards()
{
	if (!FixDifficultyRewards) return;

	DWORD addr_UIOptions_PersistableRestore = GetAddress(Addr::UIOptions_PersistableRestore);

	g_Addresses.UIFrontendManagerPtr = GetAddress(Addr::UIFrontendManagerPtr);
	g_Addresses.OptionsDifficultyPtr = GetAddress(Addr::OptionsDifficultyPtr);
	UIOptions_PersistableRestore = HookHelper::CreateHook((void*)addr_UIOptions_PersistableRestore, &UIOptions_PersistableRestore_Hook);
}
