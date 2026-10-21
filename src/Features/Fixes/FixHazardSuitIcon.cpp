#pragma once

#include "../../Globals.cpp"

// =========================
// FixHazardSuitIcon
// =========================

safetyhook::InlineHook VFXManager_LoadInit;

struct HazardIconEffect
{
	DWORD id[4];
	size_t textureNameOffset;
};

// Store pictures of the Hazard Suit (inv_suit_pdlc1 and inv_suit_pdlc1_select)
static constexpr HazardIconEffect HAZARD_ICON_EFFECTS[] =
{
	{ { 0x448C450D, 0x448C450D, 0x448C450D, 0x302755F4 }, 0x330 },
	{ { 0x847EFFC2, 0x847EFFC2, 0x847EFFC2, 0x74EB8CBB }, 0x350 },
};

static constexpr char ADVANCED_SUIT_TEXTURE[16] = { 'i', 'n', 'v', '_', 'a', 'd', 'v', '_', 'e', 'l', 'i', '_', 's', 'u', 'i', 't' };
static constexpr char ENGINEERING_SUIT_TEXTURE[16] = "inv_engsuit";

static void __fastcall VFXManager_LoadInit_Hook(uintptr_t thisp, int, uint8_t* effectData, const DWORD* resourceId)
{
	if (effectData && resourceId)
	{
		for (const HazardIconEffect& effect : HAZARD_ICON_EFFECTS)
		{
			if (!MatchId(resourceId, effect.id[0], effect.id[1], effect.id[2], effect.id[3])) continue;

			// The picture was copied from the Shockpoint Suit and still shows the Elite Advanced Suit's helmet
			char* textureName = reinterpret_cast<char*>(effectData + effect.textureNameOffset);
			if (std::memcmp(textureName, ADVANCED_SUIT_TEXTURE, sizeof(ADVANCED_SUIT_TEXTURE)) == 0)
			{
				std::memcpy(textureName, ENGINEERING_SUIT_TEXTURE, sizeof(ENGINEERING_SUIT_TEXTURE));
			}
			break;
		}
	}

	VFXManager_LoadInit.unsafe_thiscall<void>(thisp, effectData, resourceId);
}

static void ApplyFixHazardSuitIcon()
{
	if (!FixHazardSuitIcon) return;

	VFXManager_LoadInit = HookHelper::CreateHook((void*)GetAddress(Addr::VFXManager_LoadInit), &VFXManager_LoadInit_Hook);
}
