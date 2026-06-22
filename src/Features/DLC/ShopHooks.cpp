#pragma once

#include "../../Globals.cpp"

// ==============
// DLC
// ==============

safetyhook::InlineHook OpenShop;
safetyhook::InlineHook AddShopItem;
safetyhook::InlineHook IsDlcContentOwned;
safetyhook::InlineHook DlcRegistered;
safetyhook::InlineHook DlcOwnershipCheck;
safetyhook::InlineHook DlcStatusQuery;

static bool IsForcedItem(const DWORD* item)
{
	if (EnableHackerDLC)
	{
		if (MatchId(item, 0x58CB43ED, 0xEDE44FA8, 0x4E4F574B, 0x35373230) || // Hacker Suit
			MatchId(item, 0x38CB5536, 0x7AE1DC66, 0x54524542, 0x314D4152))   // Hacker Contact Beam
			return true;
	}
	if (EnableSeveredDLC)
	{
		if (MatchId(item, 0x31CB7561, 0x1B516549, 0x534F5254, 0x30314E49) || // Patrol Suit
			MatchId(item, 0x38CB673B, 0x11C92BBC, 0x54524542, 0x314D4152))   // Patrol Seeker Rifle
			return true;
	}
	if (EnableZealotDLC)
	{
		if (MatchId(item, 0x58CB5F60, 0x4BF6F5A0, 0x574F4843, 0x39323031) || // Zealot Suit
			MatchId(item, 0x38CB6733, 0xE6777830, 0x54524542, 0x314D4152))   // Zealot Force Gun
			return true;
	}
	if (EnableRivetGunDLC)
	{
		if (MatchId(item, 0x33CB4867, 0x256213EC, 0x4E4F4F4E, 0x32304E41))   // Rivet Gun
			return true;
	}

	return false;
}

static bool IsBlockedItem(const DWORD* item)
{
	if (item[2] != 0x54524542 || item[3] != 0x314D4152) // BERTRAM1
		return false;

	if (!EnableHazardPack)
	{
		if (MatchId(item, 0x38CB63F1, 0xDECD957E, 0x54524542, 0x314D4152) ||  // Hazard Suit
			MatchId(item, 0x38CB673C, 0x22E2241E, 0x54524542, 0x314D4152) ||  // Hazard Line Gun
			MatchId(item, 0x38CB63F1, 0xFA62DA20, 0x54524542, 0x314D4152) ||  // Shockpoint Suit
			MatchId(item, 0x38CB673E, 0xD90EFC1C, 0x54524542, 0x314D4152) ||  // Shockpoint Ripper
			MatchId(item, 0x38CB63F1, 0xED1EDDAA, 0x54524542, 0x314D4152) ||  // Triage Suit
			MatchId(item, 0x38CB673D, 0x3B05D03C, 0x54524542, 0x314D4152))    // Triage Javelin Gun
			return true;
	}
	if (!EnableMartialLawPack)
	{
		if (MatchId(item, 0x38CB63F2, 0x5D6D2A6A, 0x54524542, 0x314D4152) ||  // Bloody Vintage Suit
			MatchId(item, 0x38CB674E, 0xC74BFBAC, 0x54524542, 0x314D4152) ||  // Bloody Flamethrower
			MatchId(item, 0x38CB674F, 0xE63E8BAA, 0x54524542, 0x314D4152) ||  // Bloody Force Gun
			MatchId(item, 0x38CB674F, 0x57B08815, 0x54524542, 0x314D4152) ||  // Bloody Javelin Gun
			MatchId(item, 0x38CB63F2, 0x42052D6A, 0x54524542, 0x314D4152) ||  // Earthgov Security Suit
			MatchId(item, 0x38CB674B, 0x089B5095, 0x54524542, 0x314D4152) ||  // Earthgov Detonator
			MatchId(item, 0x38CB6749, 0x647FCDD0, 0x54524542, 0x314D4152) ||  // Earthgov Pulse Rifle
			MatchId(item, 0x38CB6749, 0xE9D2F65F, 0x54524542, 0x314D4152))    // Earthgov Seeker Rifle
			return true;
	}
	if (!EnableSupernovaPack)
	{
		if (MatchId(item, 0x38CB63F2, 0x2BE46F24, 0x54524542, 0x314D4152) ||  // Agility Advanced Suit
			MatchId(item, 0x38CB6747, 0xBACF1A30, 0x54524542, 0x314D4152) ||  // Agility Plasma Cutter
			MatchId(item, 0x38CB6744, 0x6F1A30B2, 0x54524542, 0x314D4152) ||  // Agility Rivet Gun
			MatchId(item, 0x38CB6748, 0x95D508D2, 0x54524542, 0x314D4152) ||  // Agility Pulse Rifle
			MatchId(item, 0x38CB63F2, 0x508D4590, 0x54524542, 0x314D4152) ||  // Forged Engineering Suit
			MatchId(item, 0x38CB674C, 0x111C7694, 0x54524542, 0x314D4152) ||  // Forged Plasma Cutter
			MatchId(item, 0x38CB674C, 0xC36C1B39, 0x54524542, 0x314D4152) ||  // Forged Line Gun
			MatchId(item, 0x38CB674D, 0x5D53841E, 0x54524542, 0x314D4152) ||  // Forged Ripper
			MatchId(item, 0x38CB63F2, 0x1C0C50E0, 0x54524542, 0x314D4152) ||  // Heavy Duty Vintage Suit
			MatchId(item, 0x38CB6743, 0x74D94CAC, 0x54524542, 0x314D4152) ||  // Heavy Duty Contact Beam
			MatchId(item, 0x38CB6741, 0x9463BFB4, 0x54524542, 0x314D4152) ||  // Heavy Duty Detonator
			MatchId(item, 0x38CB673F, 0xA89B2140, 0x54524542, 0x314D4152))    // Heavy Duty Line Gun
			return true;
	}

	return false;
}

static void __fastcall OpenShop_Hook(DWORD* thisPtr, int)
{
	g_State.isLoadingShopItems = true;
	OpenShop.unsafe_fastcall<void>(thisPtr);
	g_State.isLoadingShopItems = false;
}

static bool __stdcall DlcRegistered_Hook(int a1)
{
	if (g_State.forceCurrentItem)
		return true;

	return DlcRegistered.unsafe_stdcall<bool>(a1);
}

static char __fastcall DlcStatusQuery_Hook(DWORD* thisPtr, int, unsigned int a2, BYTE* a3)
{
	if (g_State.forceCurrentItem)
	{
		*a3 = 1;
		return 1;
	}

	return DlcStatusQuery.unsafe_thiscall<char>(thisPtr, a2, a3);
}

static char __stdcall DlcOwnershipCheck_Hook(int a1)
{
	if (g_State.forceCurrentItem)
		return 1;

	return DlcOwnershipCheck.unsafe_stdcall<char>(a1);
}

static bool __fastcall IsDlcContentOwned_Hook(int thisPtr, int, DWORD* a2)
{
	if (!g_State.isLoadingShopItems)
		return IsDlcContentOwned.unsafe_thiscall<bool>(thisPtr, a2);

	if (g_State.forceCurrentItem)
		return false;

	if (IsBlockedItem(a2))
		return true;

	return IsDlcContentOwned.unsafe_thiscall<bool>(thisPtr, a2);
}

static int __fastcall AddShopItem_Hook(int thisPtr, int, DWORD* a2)
{
	if (g_State.isLoadingShopItems && IsForcedItem(a2))
	{
		DWORD* shop = (DWORD*)thisPtr;
		DWORD origLevel = shop[14];
		DWORD origTier = shop[15];
		shop[14] = 0x7FFFFFFF;
		shop[15] = 0x7FFFFFFF;

		g_State.forceCurrentItem = true;
		int res = AddShopItem.thiscall<int>(thisPtr, a2);
		g_State.forceCurrentItem = false;

		shop[14] = origLevel;
		shop[15] = origTier;
		return res;
	}

	return AddShopItem.thiscall<int>(thisPtr, a2);
}
static void ApplyShopHooks()
{
	bool needBlock = !EnableHazardPack || !EnableMartialLawPack || !EnableSupernovaPack;
	bool needForce = EnableSeveredDLC || EnableHackerDLC || EnableZealotDLC || EnableRivetGunDLC;

	if (!needBlock && !needForce) return;

	DWORD addr_OpenShop = ScanModuleSignature(g_State.GameModule, "51 53 8B D9 83 BB F8 0A 00 00 00 89 5C 24 04 0F", "OpenShop");
	DWORD addr_AddShopItem = ScanModuleSignature(g_State.GameModule, "83 EC 28 53 55 56 8B 74 24 38 8B 06 57 8B F9 85", "AddShopItem");
	DWORD addr_IsDlcContentOwned = ScanModuleSignature(g_State.GameModule, "51 8B 41 44 53 55 33 DB 56 57 89 4C 24 10 3B C3", "IsDlcContentOwned");

	if (!addr_OpenShop || !addr_AddShopItem || !addr_IsDlcContentOwned) return;

	OpenShop = HookHelper::CreateHook((void*)addr_OpenShop, &OpenShop_Hook);
	AddShopItem = HookHelper::CreateHook((void*)addr_AddShopItem, &AddShopItem_Hook);
	IsDlcContentOwned = HookHelper::CreateHook((void*)addr_IsDlcContentOwned, &IsDlcContentOwned_Hook);

	if (needForce)
	{
		DWORD addr_DlcRegistered = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 50 E8 ?? ?? ?? 00 33 C9 83 F8 FF", "DlcRegistered");
		DWORD addr_DlcOwnershipCheck = ScanModuleSignature(g_State.GameModule, "53 B3 01 E8 ?? ?? ?? ?? 83 F8 04 75", "DlcOwnershipCheck");
		DWORD addr_DlcStatusQuery = ScanModuleSignature(g_State.GameModule, "CC CC CC CC CC CC CC CC 8B 54 24 04 32 C0 3B 91 18 03 00 00", "DlcStatusQuery");

		if (!addr_DlcRegistered || !addr_DlcOwnershipCheck || !addr_DlcStatusQuery) return;

		DlcRegistered = HookHelper::CreateHook((void*)addr_DlcRegistered, &DlcRegistered_Hook);
		DlcOwnershipCheck = HookHelper::CreateHook((void*)addr_DlcOwnershipCheck, &DlcOwnershipCheck_Hook);
		DlcStatusQuery = HookHelper::CreateHook((void*)(addr_DlcStatusQuery + 0x8), &DlcStatusQuery_Hook);
	}
}
