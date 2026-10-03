#pragma once

#include "../../Globals.cpp"

static safetyhook::MidHook ResetSite1Pre{};
static safetyhook::MidHook ResetSite2Pre{};
static safetyhook::MidHook ResetSite1Post{};
static safetyhook::MidHook ResetSite2Post{};
static safetyhook::MidHook Present1{};
static safetyhook::MidHook Present2{};

static void OnResetSitePre(safetyhook::Context&)
{
	if (FixFlareArtifacts)
	{
		ReleaseFlareFixResources();
		g_State.device = nullptr;
	}

	if (ImprovedAntiAliasingMode == AA_SMAA)
	{
		ReleaseSMAATargets();
	}

	if (AchievementSupport)
	{
		AchievementOverlay::OnDeviceLost();
	}
}

static void OnResetSitePost(safetyhook::Context&)
{
	AchievementOverlay::OnDeviceReset();
}

static void OnPresentSite(safetyhook::Context&)
{
	AchievementOverlay::OnPresent();
}

static void ApplyD3D9ApiMidHook()
{
	if (!FixFlareArtifacts && !AchievementSupport && ImprovedAntiAliasingMode != AA_SMAA && !PreloadStreamedTextures) return;

	DWORD addr_ResetSite1 = ScanModuleSignature(g_State.GameModule, "8B 42 40 FF D0 83 3D", "ResetSite1");
	DWORD addr_ResetSite2 = ScanModuleSignature(g_State.GameModule, "8B 08 8B 51 40 83 C4 04 68", "ResetSite2");

	if (addr_ResetSite1 == 0 ||
		addr_ResetSite2 == 0) {
		return;
	}

	g_Addresses.DevicePtr = MemoryHelper::ReadMemory<int>(addr_ResetSite2 - 0x4);

	ResetSite1Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1), OnResetSitePre);
	ResetSite2Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 - 0x5), OnResetSitePre);

	if (!AchievementSupport) return;

	DWORD addr_Present1 = ScanModuleSignature(g_State.GameModule, "8B 51 44 6A 00 6A 00 6A 00 6A 00", "Present1");
	DWORD addr_Present2 = ScanModuleSignature(g_State.GameModule, "8B 51 44 56 56 56 56 50", "Present2");
	DWORD addr_RenderStateFlush = ScanModuleSignature(g_State.GameModule, "8B 04 B5 ?? ?? ?? ?? 3B 04 B5 ?? ?? ?? ?? 74 ?? 50 89 04 B5", "RenderStateFlush");

	if (addr_Present1 == 0 ||
		addr_Present2 == 0 ||
		addr_RenderStateFlush == 0) {
		return;
	}

	AchievementOverlay::Init(g_Addresses.DevicePtr, MemoryHelper::ReadMemory<int>(addr_RenderStateFlush + 0xA));

	Present1 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present1), OnPresentSite);
	Present2 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present2), OnPresentSite);
	ResetSite1Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1 + 0x5), OnResetSitePost);
	ResetSite2Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 + 0x10), OnResetSitePost);
}
