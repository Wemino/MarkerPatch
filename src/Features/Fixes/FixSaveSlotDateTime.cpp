#pragma once

#include "../../Globals.cpp"

// =========================
// FixSaveSlotDateTime
// =========================

static safetyhook::MidHook FormatSaveSlotDateTime{};

static void OnFormatSaveSlotDateTime(safetyhook::Context& ctx)
{
	// Fix the 64-bit timestamp's upper bits to correctly handle dates after 2038.
	ctx.edx = *reinterpret_cast<DWORD*>(ctx.ebp + 0xC);
}

static void ApplyFixSaveSlotDateTime()
{
	if (!FixSaveSlotDateTime) return;

	FormatSaveSlotDateTime = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FormatSaveSlotDateTime)), OnFormatSaveSlotDateTime);
}
