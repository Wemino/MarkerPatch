#pragma once

#include "../../Globals.cpp"

// =========================
// FixSuitIDConflicts
// =========================

safetyhook::InlineHook InitializeItem;

static int __fastcall InitializeItem_hook(DWORD* thisp, int, int a2)
{
	int result = InitializeItem.unsafe_thiscall<int>(thisp, a2);

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

	DWORD addr_InitializeItem = ScanModuleSignature(g_State.GameModule, "83 EC 08 55 56 8B F1 57 85 F6 74", "InitializeItem");

	if (addr_InitializeItem == 0) return;

	InitializeItem = HookHelper::CreateHook((void*)addr_InitializeItem, &InitializeItem_hook);
}
