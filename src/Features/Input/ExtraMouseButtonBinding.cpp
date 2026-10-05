#pragma once

#include "../../Globals.cpp"

// ===========================
// ExtraMouseButtonBinding
// ===========================

safetyhook::InlineHook ApplyActionBinding;
safetyhook::InlineHook EvaluateKeyboardKeys;

static safetyhook::MidHook RemapVisitMappingHook{};
static safetyhook::MidHook MouseDeviceUpdateHook{};

struct ControlScheme
{
	uintptr_t keyboardMapping;
	uintptr_t mouseMapping;
	uint32_t flags;
};

struct ActionMapping
{
	uintptr_t mapping;
	uint32_t index;
};

struct ActionMappingList
{
	ActionMapping* data;
	uint32_t size;
};

static bool IsExtraMouseButton(uint32_t button)
{
	return button >= 3 && button <= 7;
}

static bool IsExtraMouseButtonKey(uint8_t key)
{
	return key >= 0xF0 + 3 && key <= 0xF0 + 7;
}

static bool IsMouseButtonDown(uint32_t button)
{
	uintptr_t state = g_State.mouseDeviceState;
	if (state == 0) return false;

	uint32_t digitalCount = *reinterpret_cast<uint32_t*>(state + 0x10);
	uint32_t analogCount = *reinterpret_cast<uint32_t*>(state + 0x14);

	if (button < digitalCount)
	{
		const uint8_t* digitalValues = *reinterpret_cast<uint8_t**>(state + 0x1C);
		return (digitalValues[button >> 3] >> (button & 7)) & 1;
	}

	if (button < digitalCount + analogCount)
	{
		const uint8_t* analogValues = *reinterpret_cast<uint8_t**>(state + 0x20);
		return analogValues[button - digitalCount] != 0;
	}

	return false;
}

static bool HasMapping(const ActionMappingList* mappings, uintptr_t mapping)
{
	for (uint32_t i = 0; i < mappings->size; i++)
	{
		if (mappings->data[i].mapping == mapping) return true;
	}

	return false;
}

static void OnRemapVisitMapping(safetyhook::Context& ctx)
{
	// 1 = Keyboard, 2 = Mouse
	uint32_t& mappingDevice = *reinterpret_cast<uint32_t*>(ctx.esp + 0x8);
	uint32_t inputDevice = *reinterpret_cast<uint32_t*>(ctx.ecx + 0x8);
	uint32_t input = *reinterpret_cast<uint32_t*>(ctx.ecx + 0x1C);

	uintptr_t remapMenu = *reinterpret_cast<uintptr_t*>(ctx.ecx + 0xC);
	uint32_t slot = *reinterpret_cast<uint32_t*>(remapMenu + 0x60);

	if (mappingDevice == 1 && inputDevice == 2 && IsExtraMouseButton(input) && slot < 2)
	{
		mappingDevice = 2;
	}
}

static void OnMouseDeviceUpdate(safetyhook::Context& ctx)
{
	if (*reinterpret_cast<uintptr_t*>(ctx.ecx + 0x4) != ctx.ecx + 0x10)
	{
		g_State.mouseDeviceState = *reinterpret_cast<uintptr_t*>(ctx.esp + 0x4);
	}
}

static int __fastcall ApplyActionBinding_Hook(uintptr_t remapper, int, const uint8_t* binding, const ActionMappingList* mappings)
{
	int result = ApplyActionBinding.thiscall<int>(remapper, binding, mappings);

	if (!binding) return result;

	const ControlScheme* schemes = *reinterpret_cast<ControlScheme**>(remapper - 0x8);
	uint32_t schemeCount = *reinterpret_cast<uint32_t*>(remapper - 0x4);

	bool hasMouseButtons = binding[8] == 1 || binding[8] == 3;

	for (uint32_t i = 0; i < mappings->size; i++)
	{
		const ActionMapping& entry = mappings->data[i];

		for (uint32_t j = 0; j < schemeCount; j++)
		{
			if (schemes[j].keyboardMapping != entry.mapping || HasMapping(mappings, schemes[j].mouseMapping)) continue;

			uint8_t* keys = reinterpret_cast<uint8_t*>(entry.mapping + 0x18 + entry.index * 8);

			for (int slot = 0; slot < 2; slot++)
			{
				uint8_t button = hasMouseButtons ? binding[9 + slot] : 0xFF;

				if (binding[slot] == 0 && IsExtraMouseButton(button))
				{
					keys[slot] = 0xF0 + button;
				}
			}
		}
	}

	return result;
}

static int __stdcall EvaluateKeyboardKeys_Hook(uintptr_t keyboardState, uint32_t keys, uint32_t keyFlags)
{
	int result = EvaluateKeyboardKeys.stdcall<int>(keyboardState, keys, keyFlags);

	for (int i = 0; i < 4 && result == 0; i++)
	{
		uint8_t key = static_cast<uint8_t>(keys >> (i * 8));

		if (IsExtraMouseButtonKey(key) && IsMouseButtonDown(key - 0xF0))
		{
			result = 0xFF;
		}
	}

	return result;
}

static void ApplyExtraMouseButtonBinding()
{
	if (!ExtraMouseButtonBinding) return;

	DWORD addr_RemapVisitMapping = ScanModuleSignature(g_State.GameModule, "83 EC 0C 55 8B 6C 24 18 56 8B F1 3B 6E 08 0F 85 ?? ?? ?? ?? 8B 44 24 18 53 50", "RemapVisitMapping");
	DWORD addr_ApplyActionBinding = ScanModuleSignature(g_State.GameModule, "83 EC 0C 56 57 8B 7C 24 1C 8B 37 8B 47 04 8D 0C C6 3B F1 74", "ApplyActionBinding");
	DWORD addr_EvaluateKeyboardKeys = ScanModuleSignature(g_State.GameModule, "83 EC 08 8B 4C 24 10 8B 54 24 14 56 57 8B 7C 24 14 33 C0", "EvaluateKeyboardKeys");
	DWORD addr_MouseDeviceUpdate = ScanModuleSignature(g_State.GameModule, "83 EC 18 8B 44 24 20 53 55 8B 6C 24 24 56 57 8B 7C 24 34 57 50 55 8B F1", "MouseDeviceUpdate");

	if (addr_RemapVisitMapping == 0 ||
		addr_ApplyActionBinding == 0 ||
		addr_EvaluateKeyboardKeys == 0 ||
		addr_MouseDeviceUpdate == 0) {
		return;
	}

	// Version 1.0 only reads the first three mouse buttons and numbers the wheel right after them
	if (MemoryHelper::ReadMemory<uint8_t>(addr_MouseDeviceUpdate + 0x17F) < 8) return;

	RemapVisitMappingHook = safetyhook::create_mid(reinterpret_cast<void*>(addr_RemapVisitMapping), OnRemapVisitMapping);
	MouseDeviceUpdateHook = safetyhook::create_mid(reinterpret_cast<void*>(addr_MouseDeviceUpdate), OnMouseDeviceUpdate);

	ApplyActionBinding = HookHelper::CreateHook((void*)addr_ApplyActionBinding, &ApplyActionBinding_Hook);
	EvaluateKeyboardKeys = HookHelper::CreateHook((void*)addr_EvaluateKeyboardKeys, &EvaluateKeyboardKeys_Hook);
}
