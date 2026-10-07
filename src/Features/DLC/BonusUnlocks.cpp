#pragma once

#include "../../Globals.cpp"

// ==============
// Bonus Unlocks
// ==============

safetyhook::InlineHook PlayerStoreSM_AddStoreListItemsToStore;
safetyhook::InlineHook PlayerStore_AddItem;
safetyhook::InlineHook UnlockedContent_IsUnlocked;
safetyhook::InlineHook AchievementManager_IsAchievementCompleteByPlatformId;

static safetyhook::MidHook SaveManagerBootCheck{};

static void(__thiscall* UnlockedContent_ForceUnlocked)(uintptr_t, unsigned int) = nullptr;
static void(__thiscall* UnlockedContent_ClearUnlocked)(uintptr_t, unsigned int) = nullptr;

struct ContentPack
{
	uint32_t key;
	const bool* enabled;
	bool isGrantedByGame;
};

static const ContentPack ContentPacks[] =
{
	{ 0xEB53E648, &EnableSupernovaPack, true },
	{ 0xEB53E649, &EnableHazardPack, true },
	{ 0x0C3DC7BC, &EnableMartialLawPack, true }, // Bloody items
	{ 0x158AC5E1, &EnableMartialLawPack, true }, // EarthGov items
	{ 0x8F1D77C2, &EnableSeveredDLC, false },
	{ 0x6DFF2805, &EnableZealotDLC, false },
	{ 0x9954AE8F, &EnableRivetGunDLC, false },
};

static void OnSaveManagerBootCheck(safetyhook::Context& ctx)
{
	if (EnableIgnitionRooms)
	{
		MemoryHelper::WriteMemory<bool>(ctx.edi + 0x641B1, true, false);
	}

	if (EnableOriginalPlasmaCutter)
	{
		MemoryHelper::WriteMemory<bool>(ctx.edi + 0x641B2, true, false);
	}

	uintptr_t unlockHandler = *reinterpret_cast<uintptr_t*>(g_Addresses.UnlockHandlerPtr);
	if (!unlockHandler) return;

	for (const ContentPack& pack : ContentPacks)
	{
		if (*pack.enabled)
		{
			UnlockedContent_ForceUnlocked(unlockHandler, pack.key);
		}
		else if (pack.isGrantedByGame)
		{
			UnlockedContent_ClearUnlocked(unlockHandler, pack.key);
		}
	}
}

static bool IsHackerItem(const DWORD* item)
{
    return MatchId(item, 0x58CB43ED, 0xEDE44FA8, 0x4E4F574B, 0x35373230) || // Hacker Suit
		   MatchId(item, 0x38CB5536, 0x7AE1DC66, 0x54524542, 0x314D4152);   // Hacker Contact Beam
}

static void __fastcall PlayerStoreSM_AddStoreListItemsToStore_Hook(DWORD* thisPtr, int)
{
	g_State.isLoadingShopItems = true;
	PlayerStoreSM_AddStoreListItemsToStore.unsafe_fastcall<void>(thisPtr);
	g_State.isLoadingShopItems = false;
}

static bool __stdcall UnlockedContent_IsUnlocked_Hook(int key)
{
	if (g_State.forceCurrentItem)
		return true;

	return UnlockedContent_IsUnlocked.unsafe_stdcall<bool>(key);
}

static char __fastcall AchievementManager_IsAchievementCompleteByPlatformId_Hook(DWORD* thisPtr, int, unsigned int achnum, BYTE* result)
{
	if (g_State.forceCurrentItem)
	{
		*result = 1;
		return 1;
	}

	return AchievementManager_IsAchievementCompleteByPlatformId.unsafe_thiscall<char>(thisPtr, achnum, result);
}

static int __fastcall PlayerStore_AddItem_Hook(int thisPtr, int, DWORD* entry)
{
	if (g_State.isLoadingShopItems && IsHackerItem(entry))
	{
		// The entry skips the chapter and round the item requires
		*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(entry) + 0x19) = true;

		g_State.forceCurrentItem = true;
		int res = PlayerStore_AddItem.thiscall<int>(thisPtr, entry);
		g_State.forceCurrentItem = false;
		return res;
	}

	return PlayerStore_AddItem.thiscall<int>(thisPtr, entry);
}

static void ApplyHackerDLC()
{
	PlayerStoreSM_AddStoreListItemsToStore = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerStoreSM_AddStoreListItemsToStore), &PlayerStoreSM_AddStoreListItemsToStore_Hook);
	PlayerStore_AddItem = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerStore_AddItem), &PlayerStore_AddItem_Hook);
	UnlockedContent_IsUnlocked = HookHelper::CreateHook((void*)GetAddress(Addr::UnlockedContent_IsUnlocked), &UnlockedContent_IsUnlocked_Hook);
	AchievementManager_IsAchievementCompleteByPlatformId = HookHelper::CreateHook((void*)GetAddress(Addr::AchievementManager_IsAchievementCompleteByPlatformId), &AchievementManager_IsAchievementCompleteByPlatformId_Hook);
}

static void ApplyBonusUnlocks()
{
	g_Addresses.UnlockHandlerPtr = GetAddress(Addr::UnlockHandlerPtr);
	UnlockedContent_ForceUnlocked = reinterpret_cast<decltype(UnlockedContent_ForceUnlocked)>(GetAddress(Addr::UnlockedContent_ForceUnlocked));
	UnlockedContent_ClearUnlocked = reinterpret_cast<decltype(UnlockedContent_ClearUnlocked)>(GetAddress(Addr::UnlockedContent_ClearUnlocked));

	SaveManagerBootCheck = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::SaveManagerBootCheck)), OnSaveManagerBootCheck);

	if (EnableHackerDLC)
	{
		ApplyHackerDLC();
	}

	if (!EnableIgnitionRooms) return;

	// The PC version always locks the Ignition doors instead of checking the flag
	MemoryHelper::WriteMemory<uint8_t>(GetAddress(Addr::IgnitionDoorSpawn), 0);
	MemoryHelper::WriteMemory<uint8_t>(GetAddress(Addr::IgnitionDoorEntitlement), 0);
}
