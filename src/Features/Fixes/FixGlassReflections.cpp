#pragma once

#include "../../Globals.cpp"
#include "../../Shaders/GlassReflectionVS.hpp"

static void ApplyFixGlassReflections()
{
	if (!FixGlassReflections) return;

	DWORD addr_VSTable = ScanModuleSignature(g_State.GameModule, "00 00 0F 80 01 00 00 02 00 08 2F 80 00 00 55 A0", "GlassVSTable");

	if (addr_VSTable == 0) return;

	uint32_t fixedVSPtr = (uint32_t)(uintptr_t)g_GlassReflectionVS;

	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x144, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x148, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x14C, fixedVSPtr);
	MemoryHelper::WriteMemory<uint32_t>(addr_VSTable + 0x154, fixedVSPtr);
}
