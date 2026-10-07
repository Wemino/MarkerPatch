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

	DWORD addr_GetGameLanguage = GetAddress(Addr::GetGameLanguage);
	DWORD addr_AchievementImpl_HandleEvents = GetAddress(Addr::AchievementImpl_HandleEvents);
	DWORD addr_AchievementManager_UnlockAchievement = GetAddress(Addr::AchievementManager_UnlockAchievement);
	DWORD addr_UpdateObtainedTrophy = GetAddress(Addr::UpdateObtainedTrophy);
	DWORD addr_AchievementImpl_PersistableRestore = GetAddress(Addr::AchievementImpl_PersistableRestore);

	GetGameLanguage = HookHelper::CreateHook((void*)addr_GetGameLanguage, &GetGameLanguage_Hook);
	AchievementImpl_HandleEvents = HookHelper::CreateHook((void*)addr_AchievementImpl_HandleEvents, &AchievementImpl_HandleEvents_Hook);
	AchievementManager_UnlockAchievement = HookHelper::CreateHook((void*)addr_AchievementManager_UnlockAchievement, &AchievementManager_UnlockAchievement_Hook);
	UpdateObtainedTrophy = HookHelper::CreateHook((void*)addr_UpdateObtainedTrophy, &UpdateObtainedTrophy_Hook);
	AchievementImpl_PersistableRestore = HookHelper::CreateHook((void*)addr_AchievementImpl_PersistableRestore, &AchievementImpl_PersistableRestore_Hook);
}
