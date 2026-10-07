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

	DWORD addr_ResetSite1 = GetAddress(Addr::ResetSite1);
	DWORD addr_ResetSite2 = GetAddress(Addr::ResetSite2);

	g_Addresses.DevicePtr = GetAddress(Addr::DevicePtr);

	ResetSite1Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1), OnResetSitePre);
	ResetSite2Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 - 0x5), OnResetSitePre);

	if (!AchievementSupport) return;

	AchievementOverlay::Init(g_Addresses.DevicePtr, GetAddress(Addr::RenderStatesPtr));

	Present1 = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::Present1)), OnPresentSite);
	Present2 = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::Present2)), OnPresentSite);
	ResetSite1Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1 + 0x5), OnResetSitePost);
	ResetSite2Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 + 0x10), OnResetSitePost);
}
