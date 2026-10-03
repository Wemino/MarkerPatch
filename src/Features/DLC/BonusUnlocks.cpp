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
		MemoryHelper::WriteMemory<bool>(ctx.edi + g_Addresses.FoundIgnitionSaveOffset, true, false);
	}

	if (EnableOriginalPlasmaCutter)
	{
		MemoryHelper::WriteMemory<bool>(ctx.edi + g_Addresses.FoundDS1SaveOffset, true, false);
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
	DWORD addr_PlayerStoreSM_AddStoreListItemsToStore = ScanModuleSignature(g_State.GameModule, "51 53 8B D9 83 BB F8 0A 00 00 00 89 5C 24 04 0F", "PlayerStoreSM_AddStoreListItemsToStore");
	DWORD addr_PlayerStore_AddItem = ScanModuleSignature(g_State.GameModule, "83 EC 28 53 55 56 8B 74 24 38 8B 06 57 8B F9 85", "PlayerStore_AddItem");
	DWORD addr_UnlockedContent_IsUnlocked = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 50 E8 ?? ?? ?? 00 33 C9 83 F8 FF", "UnlockedContent_IsUnlocked");
	DWORD addr_AchievementManager_IsAchievementCompleteByPlatformId = ScanModuleSignature(g_State.GameModule, "CC CC CC CC CC CC CC CC 8B 54 24 04 32 C0 3B 91 18 03 00 00", "AchievementManager_IsAchievementCompleteByPlatformId");

	if (addr_PlayerStoreSM_AddStoreListItemsToStore == 0 ||
		addr_PlayerStore_AddItem == 0 ||
		addr_UnlockedContent_IsUnlocked == 0 ||
		addr_AchievementManager_IsAchievementCompleteByPlatformId == 0) {
		return;
	}

	PlayerStoreSM_AddStoreListItemsToStore = HookHelper::CreateHook((void*)addr_PlayerStoreSM_AddStoreListItemsToStore, &PlayerStoreSM_AddStoreListItemsToStore_Hook);
	PlayerStore_AddItem = HookHelper::CreateHook((void*)addr_PlayerStore_AddItem, &PlayerStore_AddItem_Hook);
	UnlockedContent_IsUnlocked = HookHelper::CreateHook((void*)addr_UnlockedContent_IsUnlocked, &UnlockedContent_IsUnlocked_Hook);
	AchievementManager_IsAchievementCompleteByPlatformId = HookHelper::CreateHook((void*)(addr_AchievementManager_IsAchievementCompleteByPlatformId + 0x8), &AchievementManager_IsAchievementCompleteByPlatformId_Hook);
}

static void ApplyBonusUnlocks()
{
	DWORD addr_SaveManagerBootCheck = ScanModuleSignature(g_State.GameModule, "56 8B 35 ?? ?? ?? ?? 85 F6 74 ?? 80 BF ?? ?? ?? ?? 00 74 ?? 6A 00 6A 07 8B CE E8 ?? ?? ?? ?? 80 BF ?? ?? ?? ?? 00", "SaveManagerBootCheck");
	DWORD addr_UnlockedContent_ClearUnlocked = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 56 50 8B F1 E8 ?? ?? ?? ?? 85 C0 7C ?? 8B 8E 1C 09 00 00", "UnlockedContent_ClearUnlocked");
	DWORD addr_IgnitionDoorSpawn = ScanModuleSignature(g_State.GameModule, "80 BF A4 03 00 00 00 74 28 68 ?? ?? ?? ?? 8B CE E8 ?? ?? ?? ?? 33 C0 8B CE C6 86 ?? ?? ?? ?? 01", "IgnitionDoorSpawn");
	DWORD addr_IgnitionDoorEntitlement = ScanModuleSignature(g_State.GameModule, "80 BF A4 03 00 00 00 74 11 8B 06 8B 90 ?? ?? ?? ?? 6A 01", "IgnitionDoorEntitlement");

	if (addr_SaveManagerBootCheck == 0 ||
		addr_UnlockedContent_ClearUnlocked == 0 ||
		addr_IgnitionDoorSpawn == 0 ||
		addr_IgnitionDoorEntitlement == 0) {
		return;
	}

	g_Addresses.FoundIgnitionSaveOffset = MemoryHelper::ReadMemory<int>(addr_SaveManagerBootCheck + 0xD);
	g_Addresses.FoundDS1SaveOffset = MemoryHelper::ReadMemory<int>(addr_SaveManagerBootCheck + 0x21);

	g_Addresses.UnlockHandlerPtr = MemoryHelper::ReadMemory<int>(addr_SaveManagerBootCheck + 0x3F);
	UnlockedContent_ForceUnlocked = reinterpret_cast<decltype(UnlockedContent_ForceUnlocked)>(MemoryHelper::ResolveRelativeAddress(addr_SaveManagerBootCheck, 0x49));
	UnlockedContent_ClearUnlocked = reinterpret_cast<decltype(UnlockedContent_ClearUnlocked)>(addr_UnlockedContent_ClearUnlocked);

	SaveManagerBootCheck = safetyhook::create_mid(reinterpret_cast<void*>(addr_SaveManagerBootCheck), OnSaveManagerBootCheck);

	if (EnableHackerDLC)
	{
		ApplyHackerDLC();
	}

	if (!EnableIgnitionRooms) return;

	// The PC version always locks the Ignition doors instead of checking the flag
	MemoryHelper::WriteMemory<uint8_t>(addr_IgnitionDoorSpawn + 0x1F, 0);
	MemoryHelper::WriteMemory<uint8_t>(addr_IgnitionDoorEntitlement + 0x12, 0);
}
