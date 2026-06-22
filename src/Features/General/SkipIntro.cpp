#pragma once

#include "../../Globals.cpp"

// ====================
// SkipIntro
// ====================

safetyhook::InlineHook LoadMovie;
safetyhook::InlineHook LoadUIAnimation;

static char __fastcall LoadMovie_Hook(int thisPtr, int, const char* Source, char a3, int a4, int a5, int a6)
{
	// Skip logos video
	if (strstr(Source, "trio_frontend.vp6\x00"))
	{
		Source = "\x00";
		(void)LoadMovie.disable();
	}

	return LoadMovie.thiscall<char>(thisPtr, Source, a3, a4, a5, a6);
}

static int __stdcall LoadUIAnimation_Hook(int id)
{
	// FE66 - skip the animation playing alongside the video
	if (id == 0x46453636)
	{
		(void)LoadUIAnimation.disable();
		return 0;
	}

	return LoadUIAnimation.stdcall<int>(id);
}
static void ApplySkipIntro()
{
	if (!SkipIntro) return;

	DWORD addr_LoadUIAnimation = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 08 56 E8 ?? ?? ?? ?? 56 E8 ?? ?? ?? ?? 8B C8", "LoadUIAnimation");
	DWORD addr_LoadMovie = ScanModuleSignature(g_State.GameModule, "83 EC 10 53 55 56 33 DB 57 8B F1 88 5C 24 13 E8", "LoadMovie");

	if (addr_LoadUIAnimation == 0 ||
		addr_LoadMovie == 0) {
		return;
	}

	LoadUIAnimation = HookHelper::CreateHook((void*)addr_LoadUIAnimation, &LoadUIAnimation_Hook);
	LoadMovie = HookHelper::CreateHook((void*)addr_LoadMovie, &LoadMovie_Hook);
}
