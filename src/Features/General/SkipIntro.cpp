#pragma once

#include "../../Globals.cpp"

// ====================
// SkipIntro
// ====================

safetyhook::InlineHook RtMoviePlayer_Play;
safetyhook::InlineHook UIScreenManager_ShowScreen;

static char __fastcall RtMoviePlayer_Play_Hook(int thisPtr, int, const char* movieName, char preload, int movieAllocator, int streamBufferSize, int allocatorSize)
{
	// Skip logos video
	if (strstr(movieName, "trio_frontend.vp6\x00"))
	{
		movieName = "\x00";
		(void)RtMoviePlayer_Play.disable();
	}

	return RtMoviePlayer_Play.thiscall<char>(thisPtr, movieName, preload, movieAllocator, streamBufferSize, allocatorSize);
}

static int __stdcall UIScreenManager_ShowScreen_Hook(int signature)
{
	// FE66 - skip the animation playing alongside the video
	if (signature == 0x46453636)
	{
		(void)UIScreenManager_ShowScreen.disable();
		return 0;
	}

	return UIScreenManager_ShowScreen.stdcall<int>(signature);
}

static void ApplySkipIntro()
{
	if (!SkipIntro) return;

	DWORD addr_UIScreenManager_ShowScreen = GetAddress(Addr::UIScreenManager_ShowScreen);
	DWORD addr_RtMoviePlayer_Play = GetAddress(Addr::RtMoviePlayer_Play);

	UIScreenManager_ShowScreen = HookHelper::CreateHook((void*)addr_UIScreenManager_ShowScreen, &UIScreenManager_ShowScreen_Hook);
	RtMoviePlayer_Play = HookHelper::CreateHook((void*)addr_RtMoviePlayer_Play, &RtMoviePlayer_Play_Hook);
}
