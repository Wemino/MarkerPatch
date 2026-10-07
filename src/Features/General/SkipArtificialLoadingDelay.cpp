#pragma once

#include "../../Globals.cpp"

static void ApplySkipArtificialLoadingDelay()
{
	if (!SkipArtificialLoadingDelay) return;

	DWORD addr_SkipArtificialLoadingDelay = GetAddress(Addr::SkipArtificialLoadingDelay);

	MemoryHelper::MakeNOP(addr_SkipArtificialLoadingDelay, 2);
}
