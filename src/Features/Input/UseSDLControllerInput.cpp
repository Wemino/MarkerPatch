#pragma once

#include "../../Globals.cpp"

// =========================
// UseSDLControllerInput
// =========================

safetyhook::InlineHook XInputGetStateHook;
safetyhook::InlineHook XInputSetStateHook;

static DWORD WINAPI XInputGetState_Hook(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	if (dwUserIndex != 0) return ERROR_DEVICE_NOT_CONNECTED;
	DWORD result = ControllerHelper::PollController(pState, InvertABXYButtons);

	if (AchievementSupport && pState)
	{
		AchievementOverlay::FeedControllerState(*pState, result == ERROR_SUCCESS);

		if (AchievementOverlay::WantCaptureController())
		{
			ZeroMemory(&pState->Gamepad, sizeof(XINPUT_GAMEPAD)); // hide input from the game
		}
	}

	return result;
}

static DWORD WINAPI XInputSetState_Hook(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
	if (dwUserIndex != 0) return ERROR_DEVICE_NOT_CONNECTED;
	return ControllerHelper::SetVibration(pVibration);
}

static void ApplyUseSDLControllerInput()
{
	if (!UseSDLControllerInput) return;

	ControllerHelper::InitializeSDLGamepad();
	XInputGetStateHook = HookHelper::CreateHookAPI(L"XINPUT1_3.dll", "XInputGetState", &XInputGetState_Hook);
	XInputSetStateHook = HookHelper::CreateHookAPI(L"XINPUT1_3.dll", "XInputSetState", &XInputSetState_Hook);
}
