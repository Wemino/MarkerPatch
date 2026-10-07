#pragma once

#include "../../Globals.cpp"

// =========================
// FixSaveStringHandling
// =========================

safetyhook::InlineHook LoadSaveFileList;
safetyhook::InlineHook CopyStringFromSave;

static int __fastcall LoadSaveFileList_Hook(int thisPtr, int, const wchar_t* Source, int a3, int a4)
{
	// Replace wildcard with two-char pattern for save slots
	if (wcscmp(Source, L"ds_slot_*.deadspacesaved") == 0)
	{
		Source = L"ds_slot_??.deadspacesaved";
	}
	else if (wcscmp(Source, L"ds_slot_*.deadspace2saved") == 0)
	{
		Source = L"ds_slot_??.deadspace2saved";
	}

	return LoadSaveFileList.thiscall<int>(thisPtr, Source, a3, a4);
}

static errno_t __cdecl CopyStringFromSave_Hook(wchar_t* Destination, wchar_t* Source)
{
	if (!Source)
	{
		// NULL source
		if (Destination)
		{
			memset(Destination, 0, 0x80);
		}

		return -1;
	}

	if (!Destination)
	{
		return -1;
	}

	size_t i;
	for (i = 0; i < 0x7F; i++)
	{
		if (Source[i] == L'\0')
		{
			Destination[i] = L'\0';
			return 0;
		}

		Destination[i] = Source[i];
	}

	Destination[0x7F] = L'\0';
	return 0;
}

static void ApplyFixSaveStringHandling()
{
	if (!FixSaveStringHandling) return;

	DWORD addr_LoadSaveFileList = GetAddress(Addr::LoadSaveFileList);
	DWORD addr_CopyStringFromSave = GetAddress(Addr::CopyStringFromSave);

	LoadSaveFileList = HookHelper::CreateHook((void*)addr_LoadSaveFileList, &LoadSaveFileList_Hook);
	CopyStringFromSave = HookHelper::CreateHook((void*)addr_CopyStringFromSave, &CopyStringFromSave_Hook);
}
