#pragma once

#include "../../Globals.cpp"

static safetyhook::MidHook CPUCrashFix{};

static void OnCPUCrashFix(safetyhook::Context& ctx)
{
	uint32_t ebp = static_cast<uint32_t>(ctx.ebp);
	uint32_t* cpuCount = reinterpret_cast<uint32_t*>(ebp - 0x20);
	if (*cpuCount == 4)
	{
		uint32_t* currentAffinityMask = reinterpret_cast<uint32_t*>(ebp - 0x24);
		*currentAffinityMask = 0;
	}
}

static void ApplyHighCoreCPUFix()
{
	if (!HighCoreCPUFix) return;

	DWORD CPUFix = ScanModuleSignature(g_State.GameModule, "8B 5D D8 83 C4 18 33 FF", "CPUFix");

	if (CPUFix == 0) return;

	CPUCrashFix = safetyhook::create_mid(reinterpret_cast<void*>(CPUFix), OnCPUCrashFix);
}
