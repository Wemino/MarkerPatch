#pragma once

#include "../../Globals.cpp"

static void ApplyIncreasedDecalPersistence()
{
	if (!IncreasedDecalPersistence) return;

	DWORD addr_DecalVertexBuffer = ScanModuleSignature(g_State.GameModule, "51 6A 10 68 00 53 07 00", "DecalVertexBuffer");
	DWORD addr_CreateVertexBuffer1 = ScanModuleSignature(g_State.GameModule, "6A 00 89 76 08 6A 00 C7 46 04 00 00 10 00", "CreateVertexBuffer1");
	DWORD addr_CreateVertexBuffer2 = ScanModuleSignature(g_State.GameModule, "53 89 76 08 53 C7 46 04 00 00 10 00", "CreateVertexBuffer2");
	DWORD addr_VertexBufferSize = ScanModuleSignature(g_State.GameModule, "00 00 10 00 2B C2 3B C6 B9 00 10 00 00", "VertexBufferSize");

	if (addr_DecalVertexBuffer == 0 ||
		addr_CreateVertexBuffer1 == 0 ||
		addr_CreateVertexBuffer2 == 0 ||
		addr_VertexBufferSize == 0) {
		return;
	}

	MemoryHelper::WriteMemory<int>(addr_DecalVertexBuffer + 0x4, 1920000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0xA, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0x22, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x8, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x20, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_VertexBufferSize, 0x400000);
}
