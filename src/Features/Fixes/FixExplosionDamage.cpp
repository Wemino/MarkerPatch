#pragma once

#include "../../Globals.cpp"

// =========================
// FixExplosionDamage
// =========================

safetyhook::InlineHook TEffectEFE_Simulate;
safetyhook::InlineHook TEffectEFE_HitEntity;

static safetyhook::MidHook TEffectEFE_Constructor{};

static std::unordered_map<uintptr_t, float> g_explosionNextHit;
static uintptr_t g_explosionSkippingHits = 0;

static void OnTEffectEFE_Constructor(safetyhook::Context& ctx)
{
	g_explosionNextHit.erase(ctx.ecx);
}

static void __fastcall TEffectEFE_Simulate_Hook(uintptr_t thisp, int, float deltaTime)
{
	uintptr_t params = *reinterpret_cast<uintptr_t*>(thisp + 0x14);
	uint32_t flags = *reinterpret_cast<uint32_t*>(params);
	float speed = *reinterpret_cast<float*>(params + 0x20);
	int damageType = *reinterpret_cast<int*>(params + 0x28);

	if ((speed != 0.0f && (flags & 0x20) == 0) || damageType == 5)
	{
		TEffectEFE_Simulate.unsafe_thiscall<void>(thisp, deltaTime);
		return;
	}

	float& nextHit = g_explosionNextHit[thisp];

	if (nextHit > deltaTime * 0.5f)
	{
		g_explosionSkippingHits = thisp;
	}
	else
	{
		nextHit += TARGET_FRAME_TIME;
	}

	nextHit = std::max(nextHit - deltaTime, -TARGET_FRAME_TIME * 0.5f);

	TEffectEFE_Simulate.unsafe_thiscall<void>(thisp, deltaTime);
	g_explosionSkippingHits = 0;
}

static void __fastcall TEffectEFE_HitEntity_Hook(uintptr_t thisp, int, uintptr_t entity, const float* position, float intensity, uintptr_t collidable, int boneIndex)
{
	if (thisp == g_explosionSkippingHits) return;

	TEffectEFE_HitEntity.unsafe_thiscall<void>(thisp, entity, position, intensity, collidable, boneIndex);
}

static void ApplyFixExplosionDamage()
{
	if (!FixExplosionDamage) return;

	DWORD addr_TEffectEFE_Constructor = ScanModuleSignature(g_State.GameModule, "8B 44 24 08 56 8B F1 8B 4C 24 08 50 51 8B CE E8 ?? ?? ?? ?? 8B 4C 24 10 0F 57 C0 C7 06", "TEffectEFE_Constructor");
	DWORD addr_TEffectEFE_Simulate = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 34 53 56 8B F1 8B 5E 10 F6 83 80 00 00 00 01 57", "TEffectEFE_Simulate");
	DWORD addr_TEffectEFE_HitEntity = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 64 53 56 57 8B F1 89 74 24 24 8B 46 10 0F 28 40 70", "TEffectEFE_HitEntity");

	if (addr_TEffectEFE_Constructor == 0 ||
		addr_TEffectEFE_Simulate == 0 ||
		addr_TEffectEFE_HitEntity == 0) {
		return;
	}

	TEffectEFE_Constructor = safetyhook::create_mid(reinterpret_cast<void*>(addr_TEffectEFE_Constructor), OnTEffectEFE_Constructor);

	TEffectEFE_Simulate = HookHelper::CreateHook((void*)addr_TEffectEFE_Simulate, &TEffectEFE_Simulate_Hook);
	TEffectEFE_HitEntity = HookHelper::CreateHook((void*)addr_TEffectEFE_HitEntity, &TEffectEFE_HitEntity_Hook);
}
