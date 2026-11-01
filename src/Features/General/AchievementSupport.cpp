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
safetyhook::InlineHook AchievementManager_AddAchievement;

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
	// The Zealot is sent at the end of Severed on any difficulty, so check for Zealot like the base game does
	if (achnum == 54)
	{
		int difficulty = MemoryHelper::ReadMemory<int>(g_Addresses.OptionsDifficultyPtr);
		int lowestDifficulty = MemoryHelper::ReadMemory<int>(g_Addresses.OptionsDifficultyPtr + 0x4);
		if (difficulty != 3 || lowestDifficulty != 3) return;
	}

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

static char __fastcall AchievementManager_AddAchievement_Hook(BYTE* thisPtr, int, unsigned int achnum, int nameHash, int descHash, char unlocked, char hidden)
{
	char result = AchievementManager_AddAchievement.unsafe_thiscall<char>(thisPtr, achnum, nameHash, descHash, unlocked, hidden);

	// Only 0-50 are registered on PC, add the Severed ones so they can be unlocked and saved
	if (achnum == 50)
	{
		for (unsigned int i = 51; i <= 58; i++)
		{
			AchievementManager_AddAchievement.unsafe_thiscall<char>(thisPtr, i, 0, 0, unlocked, hidden);
		}
	}

	return result;
}

static void ApplyAchievementSupport()
{
	if (!AchievementSupport) return;

	g_Addresses.OptionsDifficultyPtr = GetAddress(Addr::OptionsDifficultyPtr);
	GetGameLanguage = HookHelper::CreateHook((void*)GetAddress(Addr::GetGameLanguage), &GetGameLanguage_Hook);
	AchievementImpl_HandleEvents = HookHelper::CreateHook((void*)GetAddress(Addr::AchievementImpl_HandleEvents), &AchievementImpl_HandleEvents_Hook);
	AchievementManager_UnlockAchievement = HookHelper::CreateHook((void*)GetAddress(Addr::AchievementManager_UnlockAchievement), &AchievementManager_UnlockAchievement_Hook);
	UpdateObtainedTrophy = HookHelper::CreateHook((void*)GetAddress(Addr::UpdateObtainedTrophy), &UpdateObtainedTrophy_Hook);
	AchievementImpl_PersistableRestore = HookHelper::CreateHook((void*)GetAddress(Addr::AchievementImpl_PersistableRestore), &AchievementImpl_PersistableRestore_Hook);
	AchievementManager_AddAchievement = HookHelper::CreateHook((void*)GetAddress(Addr::AchievementManager_AddAchievement), &AchievementManager_AddAchievement_Hook);
}