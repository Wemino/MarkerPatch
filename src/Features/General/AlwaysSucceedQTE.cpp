#pragma once

#include "../../Globals.cpp"

// =========================
// AlwaysSucceedQTE
// =========================

safetyhook::InlineHook PairedAttackCoordinator_Update;
safetyhook::InlineHook TestInputEvent_TestInput;
safetyhook::InlineHook InputContextSystem_Update;
safetyhook::InlineHook PlayerSwarmShakeoffSM_HandleStateMessage;

static int(__thiscall* AttachmentComponent_GetNumberOfSlots)(uintptr_t, int, bool) = nullptr;
static bool(__thiscall* PlayerSwarmShakeoffSM_TryToDetach)(uintptr_t) = nullptr;

static uint32_t g_swarmShakeoffLastPressTime = 0;

static int __fastcall PairedAttackCoordinator_Update_Hook(uintptr_t thisp)
{
	uint8_t flags = *reinterpret_cast<uint8_t*>(thisp + 0x68);
	int stage = *reinterpret_cast<int*>(thisp + 0x6C);
	float& breakRating = *reinterpret_cast<float*>(thisp + 0xA8);
	float breakRatingDecayRate = *reinterpret_cast<float*>(thisp + 0xAC);
	float breakThreshold = *reinterpret_cast<float*>(thisp + 0xB0);

	if (stage == 3 && (flags & 0x4) != 0 && breakThreshold >= 0.0f)
	{
		// Keep a second of decay above it so it's still reached after this update
		breakRating = std::max(breakRating, breakThreshold + std::max(breakRatingDecayRate, 0.0f));
	}

	return PairedAttackCoordinator_Update.unsafe_thiscall<int>(thisp);
}

static void __fastcall TestInputEvent_TestInput_Hook(uintptr_t thisp)
{
	if ((*reinterpret_cast<uint8_t*>(thisp + 0x64) & 1) != 0)
	{
		uintptr_t attributes = *reinterpret_cast<uintptr_t*>(thisp + 0xC);
		float& pressCount = *reinterpret_cast<float*>(thisp + 0x58);
		pressCount = std::max(pressCount, *reinterpret_cast<float*>(attributes + 0x3C) + 1.0f);
	}

	TestInputEvent_TestInput.unsafe_thiscall<void>(thisp);
}

static char __fastcall InputContextSystem_Update_Hook(uintptr_t thisp)
{
	char result = InputContextSystem_Update.unsafe_thiscall<char>(thisp);

	uintptr_t player = *reinterpret_cast<uintptr_t*>(thisp + 0x4);
	int& contextEvent = *reinterpret_cast<int*>(thisp + 0x8);

	if (contextEvent == 0 && player != 0)
	{
		uintptr_t attachmentComponent = *reinterpret_cast<uintptr_t*>(player + 0x6F8);

		if (attachmentComponent != 0 && AttachmentComponent_GetNumberOfSlots(attachmentComponent, 1, true) != 0)
		{
			contextEvent = 1;
		}
	}

	return result;
}

static char __fastcall PlayerSwarmShakeoffSM_HandleStateMessage_Hook(uintptr_t thisp, int, uint32_t simTime, float frameTime, uint32_t curFlags, unsigned int messageID, uintptr_t msgData)
{
	if (messageID == 23 && simTime - g_swarmShakeoffLastPressTime >= 33)
	{
		uint32_t maxLatency = *reinterpret_cast<uint32_t*>(thisp + 0x60);
		uint32_t& mashingExpirationTime = *reinterpret_cast<uint32_t*>(thisp + 0x64);
		int& numPresses = *reinterpret_cast<int*>(thisp + 0x68);

		g_swarmShakeoffLastPressTime = simTime;
		mashingExpirationTime = simTime + maxLatency;
		numPresses++;
		PlayerSwarmShakeoffSM_TryToDetach(thisp);
	}

	return PlayerSwarmShakeoffSM_HandleStateMessage.unsafe_thiscall<char>(thisp, simTime, frameTime, curFlags, messageID, msgData);
}

static void ApplyAlwaysSucceedQTE()
{
	if (!AlwaysSucceedQTE) return;

	AttachmentComponent_GetNumberOfSlots = reinterpret_cast<decltype(AttachmentComponent_GetNumberOfSlots)>(GetAddress(Addr::AttachmentComponent_GetNumberOfSlots));
	PlayerSwarmShakeoffSM_TryToDetach = reinterpret_cast<decltype(PlayerSwarmShakeoffSM_TryToDetach)>(GetAddress(Addr::PlayerSwarmShakeoffSM_TryToDetach));

	PairedAttackCoordinator_Update = HookHelper::CreateHook((void*)GetAddress(Addr::PairedAttackCoordinator_Update), &PairedAttackCoordinator_Update_Hook);
	TestInputEvent_TestInput = HookHelper::CreateHook((void*)GetAddress(Addr::TestInputEvent_TestInput), &TestInputEvent_TestInput_Hook);
	InputContextSystem_Update = HookHelper::CreateHook((void*)GetAddress(Addr::InputContextSystem_Update), &InputContextSystem_Update_Hook);
	PlayerSwarmShakeoffSM_HandleStateMessage = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerSwarmShakeoffSM_HandleStateMessage), &PlayerSwarmShakeoffSM_HandleStateMessage_Hook);
}
