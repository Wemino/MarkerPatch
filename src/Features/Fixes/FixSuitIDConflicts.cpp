#pragma once

#include "../../Globals.cpp"

// =========================
// FixSuitIDConflicts
// =========================

safetyhook::InlineHook PickupItem_SpawnInit;

static int __fastcall PickupItem_SpawnInit_Hook(DWORD* thisp, int, int spawnInfo)
{
	int result = PickupItem_SpawnInit.unsafe_thiscall<int>(thisp, spawnInfo);

	// Hacker Suit
	if (MatchId(thisp + 9, 0x58CB43ED, 0xEDE44FA8, 0x4E4F574B, 0x35373230))
	{
		thisp[151] = 0x317A1E59; // Don't use the unique id of the Elite Advanced Suit
	}
	// Zealot Suit
	else if (MatchId(thisp + 9, 0x58CB5F60, 0x4BF6F5A0, 0x574F4843, 0x39323031))
	{
		thisp[151] = 0x4C79DD58; // Don't use the unique id of the Security Suit
	}

	return result;
}

static void ApplyFixSuitIDConflicts()
{
	if (!FixSuitIDConflicts) return;

	DWORD addr_PickupItem_SpawnInit = GetAddress(Addr::PickupItem_SpawnInit);

	PickupItem_SpawnInit = HookHelper::CreateHook((void*)addr_PickupItem_SpawnInit, &PickupItem_SpawnInit_Hook);
}
