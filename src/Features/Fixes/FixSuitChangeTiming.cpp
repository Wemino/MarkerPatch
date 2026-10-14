#pragma once

#include "../../Globals.cpp"

// =========================
// FixSuitChangeTiming
// =========================

safetyhook::InlineHook PlayerStoreSM_UniformChangeStart;
safetyhook::InlineHook PlayerUniformManager_AttachUniformToPlayer;

static constexpr float SUIT_CHANGE_EXTRA_TIME = 0.7f;
static constexpr float SUIT_CHANGE_MIN_TIME_AFTER_LOAD = 1.0f;

static float g_suitChangeClosedTime = 6.0f;
static uintptr_t g_suitChangeStoreSM = 0;
static uintptr_t g_suitChangePlayer = 0;
static DWORD g_suitChangeStartTime = 0;

static int __fastcall PlayerStoreSM_UniformChangeStart_Hook(uintptr_t thisp)
{
	g_suitChangeStoreSM = thisp;
	g_suitChangePlayer = *reinterpret_cast<uintptr_t*>(thisp + 0x50);
	g_suitChangeStartTime = MemoryHelper::ReadMemory<DWORD>(g_Addresses.SimTimeElapsedMSecPtr);

	return PlayerStoreSM_UniformChangeStart.unsafe_thiscall<int>(thisp);
}

static void __fastcall PlayerUniformManager_AttachUniformToPlayer_Hook(uintptr_t thisp, int, uintptr_t player, const uint32_t* uniformGUID)
{
	PlayerUniformManager_AttachUniformToPlayer.unsafe_thiscall<void>(thisp, player, uniformGUID);

	// The previous suit is taken off first, without a GUID
	if (g_suitChangeStoreSM == 0 || player != g_suitChangePlayer) return;
	if (uniformGUID == nullptr || (uniformGUID[0] | uniformGUID[1] | uniformGUID[2] | uniformGUID[3]) == 0) return;

	uintptr_t storeSM = g_suitChangeStoreSM;
	g_suitChangeStoreSM = 0;

	if (*reinterpret_cast<uintptr_t*>(storeSM) != g_Addresses.PlayerStoreSMVtablePtr || *reinterpret_cast<uintptr_t*>(storeSM + 0x50) != player) return;

	DWORD now = MemoryHelper::ReadMemory<DWORD>(g_Addresses.SimTimeElapsedMSecPtr);
	float loadTime = std::clamp((now - g_suitChangeStartTime) / 1000.0f, 0.0f, g_suitChangeClosedTime - SUIT_CHANGE_MIN_TIME_AFTER_LOAD);

	float& timeInCurState = *reinterpret_cast<float*>(storeSM + 0x30);
	timeInCurState += loadTime;
}

static void ApplyFixSuitChangeTiming()
{
	if (!FixSuitChangeTiming) return;

	g_Addresses.SimTimeElapsedMSecPtr = GetAddress(Addr::SimTimeElapsedMSecPtr);
	g_Addresses.PlayerStoreSMVtablePtr = GetAddress(Addr::PlayerStoreSMVtablePtr);

	uintptr_t closedTimeOperand = GetAddress(Addr::PlayerStoreSM_UniformChangeMinTime);
	g_suitChangeClosedTime = **reinterpret_cast<float**>(closedTimeOperand) + SUIT_CHANGE_EXTRA_TIME;
	MemoryHelper::WriteMemory<uintptr_t>(closedTimeOperand, reinterpret_cast<uintptr_t>(&g_suitChangeClosedTime));

	PlayerStoreSM_UniformChangeStart = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerStoreSM_UniformChangeStart), &PlayerStoreSM_UniformChangeStart_Hook);
	PlayerUniformManager_AttachUniformToPlayer = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerUniformManager_AttachUniformToPlayer), &PlayerUniformManager_AttachUniformToPlayer_Hook);
}
