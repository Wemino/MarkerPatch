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

	DWORD addr_TEffectEFE_Constructor = GetAddress(Addr::TEffectEFE_Constructor);
	DWORD addr_TEffectEFE_Simulate = GetAddress(Addr::TEffectEFE_Simulate);
	DWORD addr_TEffectEFE_HitEntity = GetAddress(Addr::TEffectEFE_HitEntity);

	TEffectEFE_Constructor = safetyhook::create_mid(reinterpret_cast<void*>(addr_TEffectEFE_Constructor), OnTEffectEFE_Constructor);

	TEffectEFE_Simulate = HookHelper::CreateHook((void*)addr_TEffectEFE_Simulate, &TEffectEFE_Simulate_Hook);
	TEffectEFE_HitEntity = HookHelper::CreateHook((void*)addr_TEffectEFE_HitEntity, &TEffectEFE_HitEntity_Hook);
}
