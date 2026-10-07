#pragma once

#include "../../Globals.cpp"

// =====================
// AutoHideMouseCursor
// =====================

static void(__fastcall* UpdateMenuCursorFunc)(uintptr_t, float) = nullptr;

static bool IsMouseUsed(uintptr_t inputDeviceManager)
{
	return *reinterpret_cast<float*>(inputDeviceManager + 0x544) != 0.0f || // X movement
		*reinterpret_cast<float*>(inputDeviceManager + 0x548) != 0.0f ||    // Y movement
		*reinterpret_cast<float*>(inputDeviceManager + 0x54C) != 0.0f ||    // Wheel
		*reinterpret_cast<uint32_t*>(inputDeviceManager + 0x558) != 0;      // Buttons
}

static void __fastcall UpdateMenuCursorCall_Hook(uintptr_t cursor, float frameTime)
{
	uintptr_t inputDeviceManager = *reinterpret_cast<uintptr_t*>(g_Addresses.InputDeviceManagerPtr);

	if (inputDeviceManager != 0)
	{
		bool isVisible = g_State.isMouseCursorVisible;

		if (IsMouseUsed(inputDeviceManager))
		{
			isVisible = true;
		}
		else if (*reinterpret_cast<uint8_t*>(inputDeviceManager + 0x578) != 18)
		{
			isVisible = false;
		}

		if (isVisible != g_State.isMouseCursorVisible)
		{
			g_State.isMouseCursorVisible = isVisible;
			*reinterpret_cast<uint32_t*>(cursor + 0x20) |= 0x8000; // Respawn the cursor effect
		}
	}

	if (g_State.isMouseCursorVisible)
	{
		UpdateMenuCursorFunc(cursor, frameTime);
		return;
	}

	// The update only draws the cursor effect of the current cursor element, hide the element during the update only so the rest of the game still sees it
	uintptr_t& element = *reinterpret_cast<uintptr_t*>(cursor + 0x60);
	uintptr_t gameElement = element;
	element = 0;
	UpdateMenuCursorFunc(cursor, frameTime);
	element = gameElement;
}

static void ApplyAutoHideMouseCursor()
{
	if (!AutoHideMouseCursor) return;

	DWORD addr_UpdateMenuCursorCall = GetAddress(Addr::UpdateMenuCursorCall);

	DWORD addr_UpdateMenuCursor = GetAddress(Addr::UpdateMenuCursor);
	UpdateMenuCursorFunc = reinterpret_cast<decltype(UpdateMenuCursorFunc)>(addr_UpdateMenuCursor);
	g_Addresses.InputDeviceManagerPtr = GetAddress(Addr::InputDeviceManagerPtr);

	MemoryHelper::MakeCALL(addr_UpdateMenuCursorCall + 0xA, reinterpret_cast<uintptr_t>(&UpdateMenuCursorCall_Hook));
}
