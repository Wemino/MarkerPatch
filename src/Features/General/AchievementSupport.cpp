#pragma once

#include "../../Globals.cpp"

// =========================
// AchievementSupport
// =========================

safetyhook::InlineHook GetGameLanguage;
safetyhook::InlineHook TrophyCountUpdate;
safetyhook::InlineHook GiveTrophy;
safetyhook::InlineHook UpdateObtainedTrophy;
safetyhook::InlineHook LoadTrophyCounter;

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

static char __fastcall TrophyCountUpdate_Hook(int thisPtr, int, unsigned int* a2)
{
	char result = TrophyCountUpdate.unsafe_thiscall<char>(thisPtr, a2);

	int slot = AchievementOverlay::CounterSlotByHash(*a2);
	if (slot >= 0)
	{
		AchievementOverlay::UpdateCounterByHash(*a2, *(int*)(thisPtr + 20 + slot * 4));
	}

	AchievementOverlay::NotifyWeaponKill(*a2);
	return result;
}

static void __fastcall GiveTrophy_Hook(BYTE* thisPtr, int, unsigned int a2)
{
	GiveTrophy.unsafe_thiscall<void>(thisPtr, a2);

	if (AchievementOverlay::NotifyUnlock((int)a2))
	{
		*(thisPtr + 24) |= 1;
		GiveTrophy.unsafe_thiscall<void>(thisPtr, 0);
	}
}

static char __fastcall UpdateObtainedTrophy_Hook(char* thisPtr, int, unsigned int a2, char a3, char a4)
{
	char result = UpdateObtainedTrophy.unsafe_thiscall<char>(thisPtr, a2, a3, a4);
	AchievementOverlay::SetAchievementUnlocked((int)a2, a3 != 0);
	return result;
}

static char __fastcall LoadTrophyCounter_Hook(int thisPtr, int, int a2, const void* a3, int a4, int a5)
{
	char result = LoadTrophyCounter.unsafe_thiscall<char>(thisPtr, a2, a3, a4, a5);
	if (a3 && a4)
	{
		const int* counters = (const int*)a3;

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
	DWORD addr_TrophyCountUpdate = ScanModuleSignature(g_State.GameModule, "8B 15 ?? ?? ?? ?? 83 EC 20 53 33 DB 56 8B F1", "TrophyCountUpdate");
	DWORD addr_GiveTrophy = ScanModuleSignature(g_State.GameModule, "80 79 10 00 74 3A 8B 44 24 04", "GiveTrophy");
	DWORD addr_UpdateObtainedTrophy = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 F8 40 73 33 8D 44 40 06", "UpdateObtainedTrophy");
	DWORD addr_LoadTrophyCounter = ScanModuleSignature(g_State.GameModule, "83 7C 24 0C 00 75 18 68 98 00 00 00", "LoadTrophyCounter");

	if (addr_GetGameLanguage == 0 ||
		addr_TrophyCountUpdate == 0 ||
		addr_GiveTrophy == 0 ||
		addr_UpdateObtainedTrophy == 0 ||
		addr_LoadTrophyCounter == 0) {
		return;
	}

	GetGameLanguage = HookHelper::CreateHook((void*)addr_GetGameLanguage, &GetGameLanguage_Hook);
	TrophyCountUpdate = HookHelper::CreateHook((void*)addr_TrophyCountUpdate, &TrophyCountUpdate_Hook);
	GiveTrophy = HookHelper::CreateHook((void*)addr_GiveTrophy, &GiveTrophy_Hook);
	UpdateObtainedTrophy = HookHelper::CreateHook((void*)addr_UpdateObtainedTrophy, &UpdateObtainedTrophy_Hook);
	LoadTrophyCounter = HookHelper::CreateHook((void*)addr_LoadTrophyCounter, &LoadTrophyCounter_Hook);
}
