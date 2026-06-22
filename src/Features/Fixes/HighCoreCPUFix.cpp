#pragma once

#include "../../Globals.cpp"
static void ApplyHighCoreCPUFix()
{
	if (!HighCoreCPUFix) return;

	DWORD CPUFix = ScanModuleSignature(g_State.GameModule, "8B 5D D8 83 C4 18 33 FF", "CPUFix");

	if (CPUFix == 0) return;

	static SafetyHookMid CPUCrashFix{};
	CPUCrashFix = safetyhook::create_mid(reinterpret_cast<void*>(CPUFix),
		[](safetyhook::Context& ctx)
		{
			uint32_t ebp = static_cast<uint32_t>(ctx.ebp);
			uint32_t* cpuCount = reinterpret_cast<uint32_t*>(ebp - 0x20);
			if (*cpuCount == 2)
			{
				uint32_t* currentAffinityMask = reinterpret_cast<uint32_t*>(ebp - 0x24);
				*currentAffinityMask = 0;
			}
		}
	);
}
