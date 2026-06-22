#pragma once

#include "../../Globals.cpp"

// =========================
// FixDifficultyRewards
// =========================

safetyhook::InlineHook LoadGame;

static bool __fastcall LoadGame_Hook(DWORD* thisPtr, int, int a2, DWORD* a3, int a4, int a5)
{
	if (a3 && (thisPtr[81] & 0x40))
	{
		// if new game+
		if (MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) && MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) + 0x96C) == 3)
		{
			// write the update flag
			int ng_plus_diff = MemoryHelper::ReadMemory<int>(MemoryHelper::ReadMemory<DWORD>(g_Addresses.NgGamePlusPtr) + 0x978);
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr, ng_plus_diff, false);
			MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, ng_plus_diff, false);
		}
		// write the lowest difficulty flag from the save file (or not initialized, used to check for achievements)
		MemoryHelper::WriteMemory(g_Addresses.LoadedSaveMemoryPtr + 0x4, a3[14], false);
	}

	return LoadGame.thiscall<bool>(thisPtr, a2, a3, a4, a5);
}
static void ApplyFixDifficultyRewards()
{
	if (!FixDifficultyRewards) return;

	DWORD addr_LoadGame = ScanModuleSignature(g_State.GameModule, "8B ?? 34 85 ?? 74 ?? 83 ?? 6C 09 00 00 03 75", "LoadGame", 3);
	DWORD addr_LoadGameRef = ScanModuleSignature(g_State.GameModule, "8B ?? 34 85 ?? 74 ?? 83 ?? 6C 09 00 00 03 75", "LoadGame");
	DWORD addr_LoadedSaveMemoryPtr = ScanModuleSignature(g_State.GameModule, "89 15 ?? ?? ?? ?? F3 0F 10 ?? 14", "LoadedSaveMemoryPtr");

	if (addr_LoadGame == 0 ||
		addr_LoadGameRef == 0 ||
		addr_LoadedSaveMemoryPtr == 0) {
		return;
	}

	g_Addresses.NgGamePlusPtr = MemoryHelper::ReadMemory<int>(addr_LoadGameRef - 0x4);
	g_Addresses.LoadedSaveMemoryPtr = MemoryHelper::ReadMemory<int>(addr_LoadedSaveMemoryPtr + 0x2);
	LoadGame = HookHelper::CreateHook((void*)addr_LoadGame, &LoadGame_Hook);
}
