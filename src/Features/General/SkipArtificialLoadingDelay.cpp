#pragma once

#include "../../Globals.cpp"

static void ApplySkipArtificialLoadingDelay()
{
	if (!SkipArtificialLoadingDelay) return;

	MemoryHelper::MakeNOP(GetAddress(Addr::SkipArtificialLoadingDelay), 2);
}
