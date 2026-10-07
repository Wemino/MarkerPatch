#pragma once

#include "../../Globals.cpp"

static safetyhook::MidHook fontScaler1{};
static safetyhook::MidHook fontScaler2{};
static safetyhook::MidHook fontScaler3{};
static safetyhook::MidHook fontScaler4{};

static void OnFontScaler(safetyhook::Context& ctx)
{
	ctx.xmm0.f32[0] = ctx.xmm0.f32[0] * FontScalingFactor;
}

static void ApplyFontScaling()
{
	if (!FontScaling) return;

	DWORD addr_FontScaling = GetAddress(Addr::FontScaling);
	DWORD addr_FontScaling2 = GetAddress(Addr::FontScaling2);

	MemoryHelper::MakeNOP(addr_FontScaling, 8);
	MemoryHelper::MakeNOP(addr_FontScaling + 0x35, 5);

	MemoryHelper::MakeNOP(addr_FontScaling2, 5);
	MemoryHelper::MakeNOP(addr_FontScaling2 + 0x2F, 5);

	if (FontScalingFactor == 1.0f) return;

	fontScaler1 = safetyhook::create_mid(addr_FontScaling + 0x8, OnFontScaler);
	fontScaler2 = safetyhook::create_mid(addr_FontScaling + 0x3A, OnFontScaler);

	fontScaler3 = safetyhook::create_mid(addr_FontScaling2 + 0x5, OnFontScaler);
	fontScaler4 = safetyhook::create_mid(addr_FontScaling2 + 0x34, OnFontScaler);
}
