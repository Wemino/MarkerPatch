#pragma once

#include "../../Globals.cpp"
#include "../../Shaders/FlashlightOverlayVS.hpp"
#include "../../Shaders/GlassReflectionVS.hpp"
#include "../../Shaders/MeshParticleVS.hpp"
#include "../../Shaders/MotionBlurVelocityVS.hpp"
#include "../../Shaders/ScreenMeshVS.hpp"

// The game keeps a copy of each shader per depth texture format it can use for shadows (none, RAWZ, INTZ, DF24) and uses the one the GPU supports
static constexpr int SHADER_VARIANTS = 4;

static void ReplaceVertexShader(uintptr_t entries, const unsigned char* shader)
{
	auto variants = reinterpret_cast<const uint32_t*>(entries);

	// Only where every variant is the same vs_3_0 shader
	if (*reinterpret_cast<const uint32_t*>(variants[0]) != 0xFFFE0300 || std::count(variants, variants + SHADER_VARIANTS, variants[0]) != SHADER_VARIANTS) return;

	for (int variant = 0; variant < SHADER_VARIANTS; variant++)
	{
		MemoryHelper::WriteMemory<uint32_t>(entries + variant * sizeof(uint32_t), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(shader)));
	}
}

static void ApplyFixVertexNormals()
{
	if (!FixVertexNormals) return;

	DWORD addr_ShaderTable = ScanModuleSignature(g_State.GameModule, "00 00 0F 80 01 00 00 02 00 08 2F 80 00 00 55 A0", "ShaderTable");
	DWORD addr_ScreenShaderTable = ScanModuleSignature(g_State.GameModule, "00 00 E4 80 02 00 E4 80 01 00 00 02 00 08 28 80 03 00 00 A0 FF FF 00 00", "ScreenShaderTable");
	DWORD addr_MotionBlurShaderTable = ScanModuleSignature(g_State.GameModule, "05 00 55 A0 42 00 00 03 04 00 0F 80 03 00 E4 80", "MotionBlurShaderTable");

	if (addr_ShaderTable == 0 || addr_ScreenShaderTable == 0 || addr_MotionBlurShaderTable == 0) return;

	ReplaceVertexShader(addr_ShaderTable + 0x144, g_GlassReflectionVS);
	ReplaceVertexShader(addr_ShaderTable + 0x414, g_FlashlightOverlayVS);
	ReplaceVertexShader(addr_ShaderTable + 0x654, g_MeshParticleVS);
	ReplaceVertexShader(addr_ScreenShaderTable + 0x43C8, g_ScreenMeshVS);
	ReplaceVertexShader(addr_MotionBlurShaderTable + 0x798, g_MotionBlurVelocityVS);
}