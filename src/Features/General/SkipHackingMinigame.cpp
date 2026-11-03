#pragma once

#include "../../Globals.cpp"

// =========================
// SkipHackingMinigame
// =========================

safetyhook::InlineHook PlayerHackingStationSM_CheckTransition;

static void(__thiscall* PlayerHackingStationSM_MiniGameSolved)(uintptr_t) = nullptr;
static void(__thiscall* PlayerHackingStationSM_PlayMiniGameSuccessfulVFX)(uintptr_t) = nullptr;
static void(__thiscall* HackingStationMiniGame_OnSuccessfulSelection)(uintptr_t, int) = nullptr;
static void(__thiscall* HackingStationMiniGame_OnExitMessage)(uintptr_t) = nullptr;

static constexpr int TRANSITION_SEGMENT_ACTIVATED = 11;
static constexpr int TRANSITION_EXIT = 12;

static char __fastcall PlayerHackingStationSM_CheckTransition_Hook(uintptr_t thisp, int, uint32_t simTime, float frameTime, int transitionID, uintptr_t transitionData)
{
	uintptr_t miniGame = *reinterpret_cast<uintptr_t*>(thisp + 0x60);
	int& progressCount = *reinterpret_cast<int*>(thisp + 0x80);
	bool& miniGameSolved = *reinterpret_cast<bool*>(thisp + 0x84);

	// Only checked once the minigame is playing
	if (transitionID == TRANSITION_SEGMENT_ACTIVATED)
	{
		if (!miniGameSolved && miniGame != 0)
		{
			// Send what the game sends when each level and then the minigame are solved
			for (int level = progressCount; level > 0; level--)
			{
				HackingStationMiniGame_OnSuccessfulSelection(miniGame, level);
			}

			progressCount = 0;
			PlayerHackingStationSM_PlayMiniGameSuccessfulVFX(thisp);
			PlayerHackingStationSM_MiniGameSolved(thisp);
			HackingStationMiniGame_OnExitMessage(miniGame);
		}

		return 0;
	}

	if (transitionID == TRANSITION_EXIT && miniGameSolved)
	{
		return 1;
	}

	return PlayerHackingStationSM_CheckTransition.unsafe_thiscall<char>(thisp, simTime, frameTime, transitionID, transitionData);
}

static void ApplySkipHackingMinigame()
{
	if (!SkipHackingMinigame) return;

	PlayerHackingStationSM_MiniGameSolved = reinterpret_cast<decltype(PlayerHackingStationSM_MiniGameSolved)>(GetAddress(Addr::PlayerHackingStationSM_MiniGameSolved));
	PlayerHackingStationSM_PlayMiniGameSuccessfulVFX = reinterpret_cast<decltype(PlayerHackingStationSM_PlayMiniGameSuccessfulVFX)>(GetAddress(Addr::PlayerHackingStationSM_PlayMiniGameSuccessfulVFX));
	HackingStationMiniGame_OnSuccessfulSelection = reinterpret_cast<decltype(HackingStationMiniGame_OnSuccessfulSelection)>(GetAddress(Addr::HackingStationMiniGame_OnSuccessfulSelection));
	HackingStationMiniGame_OnExitMessage = reinterpret_cast<decltype(HackingStationMiniGame_OnExitMessage)>(GetAddress(Addr::HackingStationMiniGame_OnExitMessage));

	PlayerHackingStationSM_CheckTransition = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerHackingStationSM_CheckTransition), &PlayerHackingStationSM_CheckTransition_Hook);
}
