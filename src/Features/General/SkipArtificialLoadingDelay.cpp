#pragma once

#include "../../Globals.cpp"

static void ApplySkipArtificialLoadingDelay()
{
	if (!SkipArtificialLoadingDelay) return;

	DWORD addr_SkipArtificialLoadingDelay = ScanModuleSignature(g_State.GameModule, "72 29 85 C9 74 E7", "SkipArtificialLoadingDelay");

	if (addr_SkipArtificialLoadingDelay == 0) return;

	MemoryHelper::MakeNOP(addr_SkipArtificialLoadingDelay, 2);
}
