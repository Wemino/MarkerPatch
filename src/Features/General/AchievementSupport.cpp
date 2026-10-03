#pragma once

#include "../../Globals.cpp"

// =========================
// AchievementSupport
// =========================

safetyhook::InlineHook GetGameLanguage;
safetyhook::InlineHook AchievementImpl_HandleEvents;
safetyhook::InlineHook AchievementManager_UnlockAchievement;
safetyhook::InlineHook UpdateObtainedTrophy;
safetyhook::InlineHook AchievementImpl_PersistableRestore;

static int __cdecl GetGameLanguage_Hook(char* String2, size_t MaxCount)
{
	int result = GetGameLanguage.ccall<int>(String2, MaxCount);

	switch (result)
	{
		case 2: AchievementOverlay::SetLanguage("fr"); break;
		case 3: AchievementOverlay::SetLanguage("de"); break;
		case 5: AchievementOverlay::SetLanguage("it"); break;
		case 8: AchievementOverlay::SetLanguage("es"); break;
		default: AchievementOverlay::SetLanguage("en"); break;
	}

	return result;
}

static char __fastcall AchievementImpl_HandleEvents_Hook(int thisPtr, int, unsigned int* msg)
{
	char result = AchievementImpl_HandleEvents.unsafe_thiscall<char>(thisPtr, msg);

	int slot = AchievementOverlay::CounterSlotByHash(*msg);
	if (slot >= 0)
	{
		AchievementOverlay::UpdateCounterByHash(*msg, *(int*)(thisPtr + 20 + slot * 4));
	}

	AchievementOverlay::NotifyWeaponKill(*msg);
	return result;
}

static void __fastcall AchievementManager_UnlockAchievement_Hook(BYTE* thisPtr, int, unsigned int achnum)
{
	AchievementManager_UnlockAchievement.unsafe_thiscall<void>(thisPtr, achnum);

	if (AchievementOverlay::NotifyUnlock((int)achnum))
	{
		*(thisPtr + 24) |= 1;
		AchievementManager_UnlockAchievement.unsafe_thiscall<void>(thisPtr, 0);
	}
}

static char __fastcall UpdateObtainedTrophy_Hook(char* thisPtr, int, unsigned int a2, char a3, char a4)
{
	char result = UpdateObtainedTrophy.unsafe_thiscall<char>(thisPtr, a2, a3, a4);
	AchievementOverlay::SetAchievementUnlocked((int)a2, a3 != 0);
	return result;
}

static char __fastcall AchievementImpl_PersistableRestore_Hook(int thisPtr, int, int storageKey, const void* pData, int nBytes, int eventId)
{
	char result = AchievementImpl_PersistableRestore.unsafe_thiscall<char>(thisPtr, storageKey, pData, nBytes, eventId);
	if (pData && nBytes)
	{
		const int* counters = (const int*)pData;

		for (int slot = 0; slot < 38; slot++)
		{
			AchievementOverlay::InitCounterBySlot(slot, counters[slot]);
		}
	}

	return result;
}

static void ApplyAchievementSupport()
{
	if (!AchievementSupport) return;

	DWORD addr_GetGameLanguage = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 0C 57 8B 7C 24 0C 56 57 68", "GetGameLanguage");
	DWORD addr_AchievementImpl_HandleEvents = ScanModuleSignature(g_State.GameModule, "8B 15 ?? ?? ?? ?? 83 EC 20 53 33 DB 56 8B F1", "AchievementImpl_HandleEvents");
	DWORD addr_AchievementManager_UnlockAchievement = ScanModuleSignature(g_State.GameModule, "80 79 10 00 74 3A 8B 44 24 04", "AchievementManager_UnlockAchievement");
	DWORD addr_UpdateObtainedTrophy = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 F8 40 73 33 8D 44 40 06", "UpdateObtainedTrophy");
	DWORD addr_AchievementImpl_PersistableRestore = ScanModuleSignature(g_State.GameModule, "83 7C 24 0C 00 75 18 68 98 00 00 00", "AchievementImpl_PersistableRestore");

	if (addr_GetGameLanguage == 0 ||
		addr_AchievementImpl_HandleEvents == 0 ||
		addr_AchievementManager_UnlockAchievement == 0 ||
		addr_UpdateObtainedTrophy == 0 ||
		addr_AchievementImpl_PersistableRestore == 0) {
		return;
	}

	GetGameLanguage = HookHelper::CreateHook((void*)addr_GetGameLanguage, &GetGameLanguage_Hook);
	AchievementImpl_HandleEvents = HookHelper::CreateHook((void*)addr_AchievementImpl_HandleEvents, &AchievementImpl_HandleEvents_Hook);
	AchievementManager_UnlockAchievement = HookHelper::CreateHook((void*)addr_AchievementManager_UnlockAchievement, &AchievementManager_UnlockAchievement_Hook);
	UpdateObtainedTrophy = HookHelper::CreateHook((void*)addr_UpdateObtainedTrophy, &UpdateObtainedTrophy_Hook);
	AchievementImpl_PersistableRestore = HookHelper::CreateHook((void*)addr_AchievementImpl_PersistableRestore, &AchievementImpl_PersistableRestore_Hook);
}
