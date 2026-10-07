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

	DWORD addr_ShaderTable = GetAddress(Addr::ShaderTable);
	DWORD addr_ScreenShaderTable = GetAddress(Addr::ScreenShaderTable);
	DWORD addr_MotionBlurShaderTable = GetAddress(Addr::MotionBlurShaderTable);

	ReplaceVertexShader(addr_ShaderTable + 0x144, g_GlassReflectionVS);
	ReplaceVertexShader(addr_ShaderTable + 0x414, g_FlashlightOverlayVS);
	ReplaceVertexShader(addr_ShaderTable + 0x654, g_MeshParticleVS);
	ReplaceVertexShader(addr_ScreenShaderTable + 0x43C8, g_ScreenMeshVS);
	ReplaceVertexShader(addr_MotionBlurShaderTable + 0x798, g_MotionBlurVelocityVS);
}