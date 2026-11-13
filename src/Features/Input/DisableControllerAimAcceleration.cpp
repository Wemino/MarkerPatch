#pragma once

#include "../../Globals.cpp"

// ================================
// DisableControllerAimAcceleration
// ================================

safetyhook::InlineHook ControllerVelocity_Update;
safetyhook::InlineHook PlayerZGJumpSM_SampleControllerData;

static bool IsControllerActive()
{
	uintptr_t inputDeviceManager = *reinterpret_cast<uintptr_t*>(g_Addresses.InputDeviceManagerPtr);
	return inputDeviceManager != 0 && *reinterpret_cast<uint8_t*>(inputDeviceManager + 0x578) != 18;
}

static float __fastcall ControllerVelocity_Update_Hook(uintptr_t thisp, int, float frameTime, float stickMagnitude, float axisValue, float stickHeading)
{
	if (!DisableControllerAimAcceleration || !IsControllerActive())
	{
		return ControllerVelocity_Update.unsafe_thiscall<float>(thisp, frameTime, stickMagnitude, axisValue, stickHeading);
	}

	float& velocity = *reinterpret_cast<float*>(thisp);
	float& lastStickMagnitude = *reinterpret_cast<float*>(thisp + 0x4);
	float& heldTime = *reinterpret_cast<float*>(thisp + 0x8);
	float& heldTimeDecrement = *reinterpret_cast<float*>(thisp + 0xC);
	float& lastStickHeading = *reinterpret_cast<float*>(thisp + 0x10);
	const float* curve = *reinterpret_cast<float**>(thisp + 0x14);

	heldTime = curve[4];
	heldTimeDecrement = 0.0f;
	lastStickMagnitude = stickMagnitude;
	lastStickHeading = stickHeading;
	velocity = curve[5] * axisValue;

	return velocity;
}

static char __fastcall PlayerZGJumpSM_SampleControllerData_Hook(uintptr_t thisp, int, float frameTime)
{
	char result = PlayerZGJumpSM_SampleControllerData.unsafe_thiscall<char>(thisp, frameTime);

	if (DisableControllerAimAcceleration && IsControllerActive() && *reinterpret_cast<uint8_t*>(thisp + 0x194) == 0)
	{
		*reinterpret_cast<float*>(thisp + 0x84) = MemoryHelper::ReadMemory<float>(g_Addresses.ZeroGAimGainMaxPtr);
	}

	return result;
}

static void ApplyDisableControllerAimAcceleration()
{
	if (!DisableControllerAimAcceleration) return;

	g_Addresses.InputDeviceManagerPtr = GetAddress(Addr::InputDeviceManagerPtr);
	g_Addresses.ZeroGAimGainMaxPtr = GetAddress(Addr::ZeroGAimGainMaxPtr);

	ControllerVelocity_Update = HookHelper::CreateHook((void*)GetAddress(Addr::ControllerVelocity_Update), &ControllerVelocity_Update_Hook);
	PlayerZGJumpSM_SampleControllerData = HookHelper::CreateHook((void*)GetAddress(Addr::PlayerZGJumpSM_SampleControllerData), &PlayerZGJumpSM_SampleControllerData_Hook);
}
