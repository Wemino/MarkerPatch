#pragma once

#include "../../Globals.cpp"

// =========================
// BlockDirectInputDevices
// =========================

safetyhook::InlineHook IsXInputDevice;

static safetyhook::MidHook InputDeviceTypeFilter{};

static bool __cdecl IsXInputDevice_hook(DWORD* lpddi)
{
	// Skip the expensive verification, we filter out DirectInput devices elsewhere
	return false;
}

static void OnInputDeviceTypeFilter(safetyhook::Context& ctx)
{
	uint32_t eax = static_cast<uint32_t>(ctx.eax);
	if (eax == 0) return;

	uint32_t deviceIndex = *reinterpret_cast<uint32_t*>(eax);
	uint32_t& deviceType = *reinterpret_cast<uint32_t*>(eax + 0x4);

	// Allow mouse, keyboard, and all XInput controllers (slots 0-3)
	bool isAllowed = (deviceType == 2) || // Mouse
		(deviceType == 3) ||              // Keyboard
		(deviceIndex <= 3);               // XInput slots 0-3

	if (!isAllowed)
	{
		deviceType = 4; // Invalid type (skipped)
	}
}

static void ApplyFilterInputDevices()
{
	if (!BlockDirectInputDevices) return;

	DWORD addr_IsXInputDevice = GetAddress(Addr::IsXInputDevice);
	DWORD addr_InitializeInputDevice = GetAddress(Addr::InitializeInputDevice);

	IsXInputDevice = HookHelper::CreateHook((void*)addr_IsXInputDevice, &IsXInputDevice_hook);

	InputDeviceTypeFilter = safetyhook::create_mid(reinterpret_cast<void*>(addr_InitializeInputDevice), OnInputDeviceTypeFilter);
}
