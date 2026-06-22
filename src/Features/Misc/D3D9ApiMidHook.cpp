#pragma once

#include "../../Globals.cpp"
static void ApplyD3D9ApiMidHook()
{
	if (!FixFlareArtifacts && !AchievementSupport) return;

	DWORD addr_ResetSite1 = ScanModuleSignature(g_State.GameModule, "8B 42 40 FF D0 83 3D", "ResetSite1");
	DWORD addr_ResetSite2 = ScanModuleSignature(g_State.GameModule, "8B 08 8B 51 40 83 C4 04 68", "ResetSite2");

	if (addr_ResetSite1 == 0 ||
		addr_ResetSite2 == 0) {
		return;
	}

	g_Addresses.DevicePtr = MemoryHelper::ReadMemory<int>(addr_ResetSite2 - 0x4);

	static SafetyHookMid ResetSite1Pre{};
	ResetSite1Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1),
		[](safetyhook::Context&)
		{
			if (FixFlareArtifacts)
			{
				ReleaseFlareFixResources();
				g_State.device = nullptr;
			}

			if (AchievementSupport)
			{
				AchievementOverlay::OnDeviceLost();
			}
		}
	);

	static SafetyHookMid ResetSite2Pre{};
	ResetSite2Pre = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 - 0x5),
		[](safetyhook::Context&)
		{
			if (FixFlareArtifacts)
			{
				ReleaseFlareFixResources();
				g_State.device = nullptr;
			}

			if (AchievementSupport)
			{
				AchievementOverlay::OnDeviceLost();
			}
		}
	);

	if (!AchievementSupport) return;

	AchievementOverlay::Init(g_Addresses.DevicePtr);

	DWORD addr_Present1 = ScanModuleSignature(g_State.GameModule, "8B 51 44 6A 00 6A 00 6A 00 6A 00", "Present1");
	DWORD addr_Present2 = ScanModuleSignature(g_State.GameModule, "8B 51 44 56 56 56 56 50", "Present2");

	if (addr_Present1 == 0 ||
		addr_Present2 == 0) {
		return;
	}

	static SafetyHookMid Present1{};
	Present1 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present1),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnPresent();
		}
	);

	static SafetyHookMid Present2{};
	Present2 = safetyhook::create_mid(reinterpret_cast<void*>(addr_Present2),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnPresent();
		}
	);

	static SafetyHookMid ResetSite1Post{};
	ResetSite1Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite1 + 0x5),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnDeviceReset();
		}
	);

	static SafetyHookMid ResetSite2Post{};
	ResetSite2Post = safetyhook::create_mid(reinterpret_cast<void*>(addr_ResetSite2 + 0x10),
		[](safetyhook::Context&)
		{
			AchievementOverlay::OnDeviceReset();
		}
	);
}
