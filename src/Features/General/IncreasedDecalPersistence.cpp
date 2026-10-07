#pragma once

#include "../../Globals.cpp"

static void ApplyIncreasedDecalPersistence()
{
	if (!IncreasedDecalPersistence) return;

	DWORD addr_DecalVertexBuffer = GetAddress(Addr::DecalVertexBuffer);
	DWORD addr_CreateVertexBuffer1 = GetAddress(Addr::CreateVertexBuffer1);
	DWORD addr_CreateVertexBuffer2 = GetAddress(Addr::CreateVertexBuffer2);
	DWORD addr_VertexBufferSize = GetAddress(Addr::VertexBufferSize);

	MemoryHelper::WriteMemory<int>(addr_DecalVertexBuffer + 0x4, 1920000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0xA, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer1 + 0x22, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x8, 0x400000);
	MemoryHelper::WriteMemory<int>(addr_CreateVertexBuffer2 + 0x20, 0x400000);

	MemoryHelper::WriteMemory<int>(addr_VertexBufferSize, 0x400000);
}
