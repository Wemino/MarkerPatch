#pragma once

#include "../../Globals.cpp"

// =========================
// BlockDirectInputDevices
// =========================

safetyhook::InlineHook IsXInputDevice;

static bool __cdecl IsXInputDevice_hook(DWORD* lpddi)
{
	// Skip the expensive verification, we filter out DirectInput devices elsewhere
	return false;
}

static void ApplyFilterInputDevices()
{
	if (!BlockDirectInputDevices) return;

	DWORD addr_IsXInputDevice = ScanModuleSignature(g_State.GameModule, "81 EC 84 00 00 00 53 56 57 33 DB 6A 4C", "IsXInputDevice");
	DWORD addr_InitializeInputDevice = ScanModuleSignature(g_State.GameModule, "85 C0 0F 84 6F 01 00 00 8B 40 04 85 C0", "InputDeviceTypeFilter");

	if (addr_IsXInputDevice == 0 ||
		addr_InitializeInputDevice == 0) {
		return;
	}

	IsXInputDevice = HookHelper::CreateHook((void*)addr_IsXInputDevice, &IsXInputDevice_hook);

	static SafetyHookMid InputDeviceTypeFilter{};
	InputDeviceTypeFilter = safetyhook::create_mid(reinterpret_cast<void*>(addr_InitializeInputDevice),
		[](safetyhook::Context& ctx)
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
	);
}
