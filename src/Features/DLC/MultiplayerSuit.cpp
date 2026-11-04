#pragma once

#include "../../Globals.cpp"

// =========================
// MultiplayerSuit
// =========================

safetyhook::InlineHook Player_AttachModel;
safetyhook::InlineHook Player_AttachHelmet;
safetyhook::InlineHook PlayerUniformManager_LoadUniformStream;

static uintptr_t(__thiscall* Base_GetComponent)(uintptr_t, unsigned int) = nullptr;
static bool(__thiscall* MaterialSwapComponent_AddHelper)(uintptr_t, const char*, const char*) = nullptr;
static uintptr_t(__cdecl* MaterialManager_FindMaterialByName)(const char*) = nullptr;
static uintptr_t(__cdecl* CResourceManager_FindResourceById)(const uint32_t*) = nullptr;
static uint32_t(__thiscall* StreamManager_Load)(uintptr_t, const char*, float, unsigned int, const uint32_t*, const char*) = nullptr;

struct MultiplayerSuitPattern
{
	const char* stream;
	const char* helmetMaterial;
	const char* bodyMaterial;
};

struct MultiplayerSuitSkin
{
	int pattern;
	float tint[3];
};

enum MultiplayerSuitPatternId
{
	PATTERN_NONE,
	PATTERN_OLIVE_DRAB,
	PATTERN_URBAN_CAMO,
	PATTERN_DESERT_CAMO,
	PATTERN_RED,
	PATTERN_BLACK,
};

static const MultiplayerSuitPattern MultiplayerSuitPatterns[] =
{
	{ nullptr, nullptr, nullptr },
	{ "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_skin\\playersecuritymp_polivedrab.str", "skinned_playerSecurityHelmOliveDrab", "skinned_playerSecBodyOliveDrab" },
	{ "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_skin\\playersecuritymp_urbancamo1.str", "skinned_playerSecurityHelmUrbanCamo1", "skinned_playerSecBodyUrbanCamo1" },
	{ "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_skin\\playersecuritymp_desertcamo.str", "skinned_playerSecurityHelmDesertCamo1", "skinned_playerSecBodyDesertCamo1" },
	{ "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_skin\\playersecuritymp_red.str", "skinned_playerSecurityHelmRed", "skinned_playerSecBodyRed" },
	{ "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_skin\\playersecuritymp_black.str", "skinned_playerSecurityHelmBlack", "skinned_playerSecBodyBlack" },
};

// Same order as mpskins.lua
static const MultiplayerSuitSkin MultiplayerSuitSkins[] =
{
	{ PATTERN_OLIVE_DRAB, { 0.2549f, 0.2745f, 0.36078f } }, // Solid Blue
	{ PATTERN_OLIVE_DRAB, { 0.32157f, 0.32157f, 0.3020f } }, // Solid Army Green
	{ PATTERN_OLIVE_DRAB, { 0.2627f, 0.1255f, 0.1217f } }, // Solid Red
	{ PATTERN_OLIVE_DRAB, { 0.0666f, 0.0666f, 0.0666f } }, // Solid Black
	{ PATTERN_OLIVE_DRAB, { 0.4470f, 0.4353f, 0.34118f } }, // Solid Olive
	{ PATTERN_URBAN_CAMO, { 0.2549f, 0.2745f, 0.36078f } }, // Urban Camo Blue
	{ PATTERN_URBAN_CAMO, { 0.32157f, 0.32157f, 0.3020f } }, // Urban Camo Army Green
	{ PATTERN_URBAN_CAMO, { 0.2627f, 0.1255f, 0.1217f } }, // Urban Camo Red
	{ PATTERN_URBAN_CAMO, { 0.639f, 0.4706f, 0.3109f } }, // Urban Camo Military
	{ PATTERN_URBAN_CAMO, { 0.4470f, 0.4353f, 0.34118f } }, // Urban Camo Olive
	{ PATTERN_DESERT_CAMO, { 0.2549f, 0.2745f, 0.36078f } }, // Tiger Camo Blue
	{ PATTERN_DESERT_CAMO, { 0.32157f, 0.32157f, 0.3020f } }, // Tiger Camo Army Green
	{ PATTERN_DESERT_CAMO, { 0.2627f, 0.1255f, 0.1217f } }, // Tiger Camo Red
	{ PATTERN_DESERT_CAMO, { 0.639f, 0.4706f, 0.3109f } }, // Tiger Camo Military
	{ PATTERN_NONE, { 1.0f, 1.0f, 1.0f } }, // War Torn
	{ PATTERN_RED, { 1.0f, 1.0f, 1.0f } }, // Visceral
	{ PATTERN_BLACK, { 1.0f, 1.0f, 1.0f } }, // Collector's Edition
	{ PATTERN_OLIVE_DRAB, { 0.2588f, 0.0f, 0.0f } }, // Dr Pepper Maroon
	{ PATTERN_URBAN_CAMO, { 0.2588f, 0.0f, 0.0f } }, // Dr Pepper Urban Camo
	{ PATTERN_DESERT_CAMO, { 0.2588f, 0.0f, 0.0f } }, // Dr Pepper Tiger
	{ PATTERN_OLIVE_DRAB, { 0.055f, 0.1529f, 0.4392f } }, // Viral Solid Blue
	{ PATTERN_URBAN_CAMO, { 0.055f, 0.1529f, 0.4392f } }, // Viral Urban Blue
	{ PATTERN_DESERT_CAMO, { 0.055f, 0.1529f, 0.4392f } }, // Viral Tiger Blue
	{ PATTERN_OLIVE_DRAB, { 0.866f, 0.9176f, 0.06274f } }, // Viral Solid Yellow
	{ PATTERN_URBAN_CAMO, { 0.866f, 0.9176f, 0.06274f } }, // Viral Urban Yellow
	{ PATTERN_DESERT_CAMO, { 0.866f, 0.9176f, 0.06274f } }, // Viral Tiger Yellow
	{ PATTERN_OLIVE_DRAB, { 1.0f, 1.0f, 1.0f } }, // Viral Solid White
	{ PATTERN_URBAN_CAMO, { 1.0f, 1.0f, 1.0f } }, // Viral Urban White
	{ PATTERN_DESERT_CAMO, { 1.0f, 1.0f, 1.0f } }, // Viral Tiger White
	{ PATTERN_URBAN_CAMO, { 0.85098f, 0.945098f, 0.654902f } }, // Matchett
	{ PATTERN_DESERT_CAMO, { 0.9882f, 0.50588f, 0.9490f } }, // Gascoigne
	{ PATTERN_OLIVE_DRAB, { 0.94118f, 0.29803f, 0.0f } }, // Probst
	{ PATTERN_OLIVE_DRAB, { 0.17647f, 0.48235f, 0.77647f } }, // Woldman
	{ PATTERN_OLIVE_DRAB, { 0.86666f, 0.86666f, 0.86666f } }, // Papoutsis
	{ PATTERN_DESERT_CAMO, { 0.8745f, 0.7373f, 0.2666f } }, // Murphy
	{ PATTERN_DESERT_CAMO, { 0.6941f, 0.4313f, 0.0117f } }, // Lu
};

static constexpr const char* SOLDIER_UNIFORM_STREAM = "global_mp_assets\\global_mp_assets\\ds_mp_assets\\char_streams\\plyr\\player_uniform\\playersecuritymp.str";
static constexpr const char* SOLDIER_HELMET_MATERIAL = "skinned_playerSecurityHelmetMP";
static constexpr const char* SOLDIER_BODY_MATERIAL = "skinned_playerSecurityBodyMP";
static constexpr uint32_t SOLDIER_MODEL[4] = { 0xAD33EB25, 0xAD33EB25, 0xAD33EB25, 0x940F702F }; // playersecuritymp.meta.xml
static constexpr uint32_t NO_HELMET[4] = { 0, 0, 0, 0 };

static uintptr_t g_multiplayerSuitHelmetPlayer = 0;

static const MultiplayerSuitSkin& GetMultiplayerSuitSkin()
{
	return MultiplayerSuitSkins[MultiplayerSuit - 1];
}

static const MultiplayerSuitPattern& GetMultiplayerSuitPattern()
{
	return MultiplayerSuitPatterns[GetMultiplayerSuitSkin().pattern];
}

static bool IsMultiplayerSuitLoaded()
{
	const MultiplayerSuitPattern& pattern = GetMultiplayerSuitPattern();

	if (CResourceManager_FindResourceById(SOLDIER_MODEL) == 0) return false;
	if (!pattern.stream) return true;

	return MaterialManager_FindMaterialByName(SOLDIER_HELMET_MATERIAL) != 0 &&
		   MaterialManager_FindMaterialByName(SOLDIER_BODY_MATERIAL) != 0 &&
		   MaterialManager_FindMaterialByName(pattern.helmetMaterial) != 0 &&
		   MaterialManager_FindMaterialByName(pattern.bodyMaterial) != 0;
}

static void __fastcall Player_AttachModel_Hook(uintptr_t thisp, int, const uint32_t* modelGUID)
{
	uintptr_t uniform = *reinterpret_cast<uintptr_t*>(thisp + 0x1688);

	// Keep the straitjacket
	if (uniform == 0 || modelGUID != reinterpret_cast<const uint32_t*>(*reinterpret_cast<uintptr_t*>(uniform + 0xC) + 0x398) ||
		MatchId(reinterpret_cast<const DWORD*>(uniform + 0x24), 0x30C9FF4E, 0x44D00498, 0x54544950, 0x32303153)) // Straitjacket id
	{
		Player_AttachModel.unsafe_thiscall<void>(thisp, modelGUID);
		return;
	}

	uint32_t playerFlags = *reinterpret_cast<uint32_t*>(thisp + 0x3D8);
	uintptr_t materialSwap = Base_GetComponent(thisp, MemoryHelper::ReadMemory<uint8_t>(g_Addresses.MaterialSwapComponentIDPtr));
	uintptr_t shaderTint = Base_GetComponent(thisp, MemoryHelper::ReadMemory<uint8_t>(g_Addresses.ShaderTintComponentIDPtr));

	if ((playerFlags & 0x2000) != 0 || materialSwap == 0 || shaderTint == 0 || !IsMultiplayerSuitLoaded())
	{
		Player_AttachModel.unsafe_thiscall<void>(thisp, modelGUID);
		return;
	}

	const MultiplayerSuitPattern& pattern = GetMultiplayerSuitPattern();

	// Cleared since the player keeps its list from one suit to the next
	*reinterpret_cast<uint16_t*>(materialSwap + 0x14) = 0;

	if (pattern.stream)
	{
		MaterialSwapComponent_AddHelper(materialSwap, SOLDIER_HELMET_MATERIAL, pattern.helmetMaterial);
		MaterialSwapComponent_AddHelper(materialSwap, SOLDIER_BODY_MATERIAL, pattern.bodyMaterial);
	}

	float* tintColor = reinterpret_cast<float*>(shaderTint + 0x20);
	tintColor[0] = GetMultiplayerSuitSkin().tint[0];
	tintColor[1] = GetMultiplayerSuitSkin().tint[1];
	tintColor[2] = GetMultiplayerSuitSkin().tint[2];

	g_multiplayerSuitHelmetPlayer = thisp;

	Player_AttachModel.unsafe_thiscall<void>(thisp, SOLDIER_MODEL);
}

static void __fastcall Player_AttachHelmet_Hook(uintptr_t thisp, int, const uint32_t* helmetGUID)
{
	if (thisp == g_multiplayerSuitHelmetPlayer)
	{
		g_multiplayerSuitHelmetPlayer = 0;
		Player_AttachHelmet.unsafe_thiscall<void>(thisp, NO_HELMET);
		return;
	}

	Player_AttachHelmet.unsafe_thiscall<void>(thisp, helmetGUID);
}

static bool __fastcall PlayerUniformManager_LoadUniformStream_Hook(uintptr_t thisp, int, uintptr_t uniformEntry)
{
	uintptr_t streamManager = *reinterpret_cast<uintptr_t*>(g_Addresses.StreamManagerPtr);

	// Queued before the suit's own stream so they are loaded by the time it is put on
	StreamManager_Load(streamManager, SOLDIER_UNIFORM_STREAM, 0.0f, 0, nullptr, nullptr);

	if (GetMultiplayerSuitPattern().stream)
	{
		StreamManager_Load(streamManager, GetMultiplayerSuitPattern().stream, 0.0f, 0, nullptr, nullptr);
	}

	return PlayerUniformManager_LoadUniformStream.unsafe_thiscall<bool>(thisp, uniformEntry);
}

static void ApplyMultiplayerSuit()
{
	if (MultiplayerSuit == 0) return;

	g_Addresses.StreamManagerPtr = GetAddress(Addr::StreamManagerPtr);
	g_Addresses.MaterialSwapComponentIDPtr = GetAddress(Addr::MaterialSwapComponentIDPtr);
	g_Addresses.ShaderTintComponentIDPtr = GetAddress(Addr::ShaderTintComponentIDPtr);

	Base_GetComponent = reinterpret_cast<decltype(Base_GetComponent)>(GetAddress(Addr::Base_GetComponent));
	MaterialSwapComponent_AddHelper = reinterpret_cast<decltype(MaterialSwapComponent_AddHelper)>(GetAddress(Addr::MaterialSwapComponent_AddHelper));
	MaterialManager_FindMaterialByName = reinterpret_cast<decltype(MaterialManager_FindMaterialByName)>(GetAddress(Addr::MaterialManager_FindMaterialByName));
	CResourceManager_FindResourceById = reinterpret_cast<decltype(CResourceManager_FindResourceById)>(GetAddress(Addr::CResourceManager_FindResourceById));
	StreamManager_Load = reinterpret_cast<decltype(StreamManager_Load)>(GetAddress(Addr::StreamManager_Load));

	Player_AttachModel = HookHelper::CreateHook((void*)GetAddress(Addr::Player_AttachModel), &Player_AttachModel_Hook);
	Player_AttachHelmet = HookHelper::CreateHook((void*)GetAddress(Addr::Player_AttachHelmet), &Player_AttachHelmet_Hook);
	PlayerUniformManager_LoadUniformStream = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerUniformManager_LoadUniformStream), &PlayerUniformManager_LoadUniformStream_Hook);
}
