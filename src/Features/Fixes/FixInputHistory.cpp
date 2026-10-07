#pragma once

#include "../../Globals.cpp"

// =========================
// FixInputHistory
// =========================

safetyhook::InlineHook ControllerManager_UpdateOneController;

struct ControllerManagerRecord
{
	uint32_t stickDirs;
	uint32_t buttonMask;
	uint32_t numberMs;
	uint32_t pad;
};

static int __fastcall ControllerManager_UpdateOneController_Hook(uintptr_t thisp, int, uintptr_t pControllerInfo, uint8_t controllerID)
{
	uintptr_t info = thisp + 0x10 + controllerID * 0x168;
	ControllerManagerRecord* arrayOfRecords = reinterpret_cast<ControllerManagerRecord*>(info + 0x58);
	int& lastRecordToUse = *reinterpret_cast<int*>(info + 0x158);

	int lastRecord = lastRecordToUse;
	int nextRecord = (lastRecord + 1) % 16;
	ControllerManagerRecord previous = arrayOfRecords[lastRecord];
	ControllerManagerRecord overwritten = arrayOfRecords[nextRecord];

	int result = ControllerManager_UpdateOneController.unsafe_thiscall<int>(thisp, pControllerInfo, controllerID);

	// The 16 records only cover a few ms when the mouse moves the stick every frame, keep stick changes at least a 30 FPS frame apart
	if (lastRecordToUse == nextRecord && arrayOfRecords[nextRecord].buttonMask == previous.buttonMask && previous.numberMs < 33)
	{
		arrayOfRecords[lastRecord].stickDirs = arrayOfRecords[nextRecord].stickDirs;
		arrayOfRecords[lastRecord].numberMs += arrayOfRecords[nextRecord].numberMs;
		arrayOfRecords[nextRecord] = overwritten;
		lastRecordToUse = lastRecord;
	}

	return result;
}

static void ApplyFixInputHistory()
{
	if (!FixInputHistory) return;

	ControllerManager_UpdateOneController = HookHelper::CreateHook((void*)GetAddress(Addr::ControllerManager_UpdateOneController), &ControllerManager_UpdateOneController_Hook);
}
