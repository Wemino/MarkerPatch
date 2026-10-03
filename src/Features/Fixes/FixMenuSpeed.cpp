#pragma once

#include "../../Globals.cpp"

// =========================
// FixMenuSpeed
// =========================

safetyhook::InlineHook UIMenuBase_MenuControlThread;
safetyhook::InlineHook PlayerTweakCameraModifier_Update;

static safetyhook::MidHook AptUpdate{};

static constexpr float MENU_EASING = 0.25f;
static constexpr float MOUSE_TILT_FADE = 0.75f;

static void OnAptUpdate(safetyhook::Context& ctx)
{
	float frameTime = MemoryHelper::ReadMemory<float>(g_Addresses.FrameTimeSecPtr);
	uint32_t& aptTimeMSec = *reinterpret_cast<uint32_t*>(ctx.esp);

	// Apt counts whole milliseconds, the rest of the frame time is kept instead of rounded away
	g_State.aptTime += frameTime * 1000.0;
	aptTimeMSec = static_cast<uint32_t>(g_State.aptTime);
	g_State.aptTime -= aptTimeMSec;

	g_State.menuFrameSteps = frameTime / TARGET_FRAME_TIME;
	g_State.menuStepTime += frameTime;
	g_State.isMenuStep = g_State.menuStepTime >= TARGET_FRAME_TIME;

	if (g_State.isMenuStep)
	{
		g_State.menuStepTime = std::min(g_State.menuStepTime - TARGET_FRAME_TIME, TARGET_FRAME_TIME);
	}
}

static int __fastcall UIMenuBase_MenuControlThread_Hook(uintptr_t thisp)
{
	if (thisp == 0) 
		return UIMenuBase_MenuControlThread.unsafe_thiscall<int>(thisp);

	uintptr_t menu = thisp + 0x5C;
	float& menuPos = *reinterpret_cast<float*>(menu + 0x54);
	float& targetMenuPos = *reinterpret_cast<float*>(menu + 0x58);
	int& animFrames = *reinterpret_cast<int*>(menu + 0x60);
	uint8_t& delayActivateItem = *reinterpret_cast<uint8_t*>(menu + 0x80);

	float startMenuPos = menuPos;
	float startTargetMenuPos = targetMenuPos;
	int startAnimFrames = animFrames;
	uint8_t wasActivateItemDelayed = delayActivateItem;

	if (!g_State.isMenuStep && wasActivateItemDelayed != 0)
	{
		delayActivateItem = 0;
	}

	int result = UIMenuBase_MenuControlThread.unsafe_thiscall<int>(thisp);

	if (!g_State.isMenuStep && delayActivateItem == 0)
	{
		delayActivateItem = wasActivateItemDelayed;
	}

	if (animFrames == startAnimFrames - 1)
	{
		float menuVel = *reinterpret_cast<float*>(menu + 0x5C);

		if (menuVel == 0.0f)
		{
			menuPos = startMenuPos + (startTargetMenuPos - startMenuPos) * (1.0f - powf(1.0f - MENU_EASING, g_State.menuFrameSteps));
		}
		else
		{
			menuPos = startMenuPos + menuVel * g_State.menuFrameSteps;
			targetMenuPos = menuPos;
		}

		if (!g_State.isMenuStep)
		{
			animFrames = startAnimFrames;
		}

		if (animFrames <= 0)
		{
			menuPos = targetMenuPos;
		}
	}

	return result;
}

static void __fastcall PlayerTweakCameraModifier_Update_Hook(uintptr_t thisp, uintptr_t, float frameTime, float fBlendFraction, uintptr_t unmodifiedData, uintptr_t inout_currentData)
{
	float* modRates = reinterpret_cast<float*>(thisp + 0x24);
	float* tiltDampenings = reinterpret_cast<float*>(thisp + 0x2C);
	float savedModRates[2] = { modRates[0], modRates[1] };
	float savedTiltDampenings[2] = { tiltDampenings[0], tiltDampenings[1] };
	float steps = frameTime / TARGET_FRAME_TIME;

	if (frameTime > 0.0f)
	{
		for (int i = 0; i < 2; i++)
		{
			modRates[i] *= steps;

			if (tiltDampenings[i] > 0.0f)
			{
				tiltDampenings[i] = powf(tiltDampenings[i], steps);
			}
		}
	}

	PlayerTweakCameraModifier_Update.unsafe_thiscall<void>(thisp, frameTime, fBlendFraction, unmodifiedData, inout_currentData);

	memcpy(modRates, savedModRates, sizeof(savedModRates));
	memcpy(tiltDampenings, savedTiltDampenings, sizeof(savedTiltDampenings));

	if (frameTime > 0.0f && *reinterpret_cast<float*>(thisp + 0x44) == 0.0f)
	{
		float* mouseTilt = reinterpret_cast<float*>(thisp + 0x3C);
		float fade = powf(MOUSE_TILT_FADE, steps - 1.0f);

		mouseTilt[0] *= fade;
		mouseTilt[1] *= fade;
	}
}

static void ApplyFixMenuSpeed()
{
	if (!FixMenuSpeed) return;

	DWORD addr_AptUpdate = ScanModuleSignature(g_State.GameModule, "D9 05 ?? ?? ?? ?? D8 0D ?? ?? ?? ?? D9 7C 24 14 0F B7 44 24 14 D8 05 ?? ?? ?? ?? 0D 00 0C 00 00 89 44 24 08 D9 6C 24 08 DF 7C 24 08 8B 54 24 08 52 D9 6C 24 18 E8", "AptUpdate");
	DWORD addr_UIMenuBase_MenuControlThread = ScanModuleSignature(g_State.GameModule, "83 C1 5C E8 ?? ?? ?? ?? 33 C0 C3", "UIMenuBase_MenuControlThread");
	DWORD addr_PlayerTweakCameraModifier_Update = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC D4 00 00 00 0F 57 C0 A1 ?? ?? ?? ?? 53 8B 1D ?? ?? ?? ?? 56 8B F1", "PlayerTweakCameraModifier_Update");

	if (addr_AptUpdate == 0 ||
		addr_UIMenuBase_MenuControlThread == 0 ||
		addr_PlayerTweakCameraModifier_Update == 0) {
		return;
	}

	g_Addresses.FrameTimeSecPtr = MemoryHelper::ReadMemory<int>(addr_AptUpdate + 0x2);

	AptUpdate = safetyhook::create_mid(reinterpret_cast<void*>(addr_AptUpdate + 0x35), OnAptUpdate);

	UIMenuBase_MenuControlThread = HookHelper::CreateHook((void*)addr_UIMenuBase_MenuControlThread, &UIMenuBase_MenuControlThread_Hook);
	PlayerTweakCameraModifier_Update = HookHelper::CreateHook((void*)addr_PlayerTweakCameraModifier_Update, &PlayerTweakCameraModifier_Update_Hook);
}
