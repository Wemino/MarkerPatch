#pragma once

#include "../../Globals.cpp"

// ==========================
// IncreasedEntityPersistence
// ==========================

safetyhook::InlineHook ResizeEntityBuffer;

static int __fastcall ResizeEntityBuffer_Hook(char* thisp, int, int bufferType, int newLimit)
{
	// The game want to clean up the array
	if (newLimit == 0)
	{
		return ResizeEntityBuffer.thiscall<int>(thisp, bufferType, newLimit);
	}

	if (bufferType == 0 && IncreasedEntityPersistenceBodies != 0) // bodies
	{
		newLimit = IncreasedEntityPersistenceBodies;
	}

	if (bufferType == 1 && IncreasedEntityPersistenceLimbs != 0) // limbs
	{
		newLimit = IncreasedEntityPersistenceLimbs;
	}

	return ResizeEntityBuffer.thiscall<int>(thisp, bufferType, newLimit);
}

static void ApplyIncreasedEntityPersistence()
{
	if (!IncreasedEntityPersistence) return;

	DWORD addr_ResizeEntityBuffer = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 83 EC 14 55 56 8D 04 40 8D 2C C1 57", "ResizeEntityBuffer");
	DWORD addr_ResizeEntityBuffer_Init = ScanModuleSignature(g_State.GameModule, "51 53 33 DB 55 56 57 8B F9 89 3D", "EntityBuffer_Init");

	if (addr_ResizeEntityBuffer == 0 ||
		addr_ResizeEntityBuffer_Init == 0) {
		return;
	}

	if (IncreasedEntityPersistenceBodies != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x34, IncreasedEntityPersistenceBodies);
		MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x7A, IncreasedEntityPersistenceBodies);
	}

	if (IncreasedEntityPersistenceLimbs != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_ResizeEntityBuffer_Init + 0x86, IncreasedEntityPersistenceLimbs);
		MemoryHelper::WriteMemory<int>(addr_ResizeEntityBuffer_Init + 0x91, IncreasedEntityPersistenceLimbs);
	}

	ResizeEntityBuffer = HookHelper::CreateHook((void*)addr_ResizeEntityBuffer, &ResizeEntityBuffer_Hook);
}
