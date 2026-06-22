#pragma once

#include "../../Globals.cpp"
static void ApplyFontScaling()
{
	if (!FontScaling) return;

	DWORD addr_FontScaling = ScanModuleSignature(g_State.GameModule, "0F 2F C1 76 03 0F 28 C1 F3 0F 10 5E 18", "FontScaling");
	DWORD addr_FontScaling2 = ScanModuleSignature(g_State.GameModule, "76 03 0F 28 C2 0F B7 0D", "FontScaling2");

	if (addr_FontScaling == 0 ||
		addr_FontScaling2 == 0) {
		return;
	}

	MemoryHelper::MakeNOP(addr_FontScaling, 8);
	MemoryHelper::MakeNOP(addr_FontScaling + 0x35, 5);

	MemoryHelper::MakeNOP(addr_FontScaling2, 5);
	MemoryHelper::MakeNOP(addr_FontScaling2 + 0x2F, 5);

	if (FontScalingFactor == 1.0f) return;

	static SafetyHookMid fontScaler1{};
	fontScaler1 = safetyhook::create_mid(addr_FontScaling + 0x8,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm0.f32[0] = ctx.xmm0.f32[0] * FontScalingFactor;
		}
	);

	static SafetyHookMid fontScaler2{};
	fontScaler2 = safetyhook::create_mid(addr_FontScaling + 0x3A,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm0.f32[0] = ctx.xmm0.f32[0] * FontScalingFactor;
		}
	);
}
