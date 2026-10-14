#pragma once

#include "../../Globals.cpp"

// =========================
// FixFlareArtifacts
// =========================

static safetyhook::MidHook FlareOcclusionResolve{};
static safetyhook::MidHook FlareOcclusionInitResolve{};
static safetyhook::MidHook FlareOcclusionClearResolve{};

static DWORD(__cdecl* TX_AllocTexture)(DWORD format, DWORD flags, int width, int height, int mipLevels, DWORD handle) = nullptr;

static constexpr uintptr_t TEXTURE_ENTRY_SIZE = 60;
static constexpr DWORD TEXTURE_HANDLE_COUNT = 0x1000;

// What the flare vertex shaders read instead of vfx_visibility, a copy made by the game's texture manager
static DWORD g_flareVisibilityCopy = 0;

static uintptr_t GetTextureEntry(DWORD handle)
{
	return g_Addresses.TextureTablePtr + handle * TEXTURE_ENTRY_SIZE;
}

static IDirect3DTexture9* GetTexture(DWORD handle)
{
	return *reinterpret_cast<IDirect3DTexture9**>(GetTextureEntry(handle) + 0x18);
}

static bool IsFlareVisibilityCopyValid(uintptr_t source)
{
	if (g_flareVisibilityCopy == 0 || g_flareVisibilityCopy >= TEXTURE_HANDLE_COUNT) return false;

	// Still allocated, and still a texture like vfx_visibility (size, format, flags)
	uintptr_t copy = GetTextureEntry(g_flareVisibilityCopy);
	return (*reinterpret_cast<uint8_t*>(copy) & 7) >= 2 &&
		*reinterpret_cast<uint32_t*>(copy + 0x2) == *reinterpret_cast<uint32_t*>(source + 0x2) &&
		*reinterpret_cast<uint32_t*>(copy + 0xC) == *reinterpret_cast<uint32_t*>(source + 0xC) &&
		*reinterpret_cast<uint32_t*>(copy + 0x10) == *reinterpret_cast<uint32_t*>(source + 0x10);
}

static void CopyFlareVisibility()
{
	DWORD visibility = MemoryHelper::ReadMemory<DWORD>(g_Addresses.FlareVisibilityTexturePtr);
	if (visibility == 0 || visibility >= TEXTURE_HANDLE_COUNT) return;

	uintptr_t source = GetTextureEntry(visibility);

	if (!IsFlareVisibilityCopyValid(source))
	{
		DWORD copy = TX_AllocTexture(*reinterpret_cast<DWORD*>(source + 0xC), *reinterpret_cast<DWORD*>(source + 0x10), *reinterpret_cast<uint16_t*>(source + 0x2), *reinterpret_cast<uint16_t*>(source + 0x4), *reinterpret_cast<uint8_t*>(source + 0x1), 0xFFFFFFFF);

		// Without a copy the shaders read vfx_visibility as before
		g_flareVisibilityCopy = copy < TEXTURE_HANDLE_COUNT ? copy : visibility;
	}

	if (g_flareVisibilityCopy == visibility) return;

	IDirect3DDevice9* device = GetD3D9Device();
	IDirect3DTexture9* sourceTexture = GetTexture(visibility);
	IDirect3DTexture9* copyTexture = GetTexture(g_flareVisibilityCopy);
	if (!device || !sourceTexture || !copyTexture) return;

	// Nvidia's driver returns garbage to vertex shaders reading a texture that was just rendered to, read a copy like the console's resolve made
	IDirect3DSurface9* sourceSurface = nullptr;
	IDirect3DSurface9* copySurface = nullptr;

	if (SUCCEEDED(sourceTexture->GetSurfaceLevel(0, &sourceSurface)) && SUCCEEDED(copyTexture->GetSurfaceLevel(0, &copySurface)))
	{
		device->StretchRect(sourceSurface, nullptr, copySurface, nullptr, D3DTEXF_NONE);
	}

	if (sourceSurface) sourceSurface->Release();
	if (copySurface) copySurface->Release();
}

static void OnFlareOcclusionResolve(safetyhook::Context&)
{
	CopyFlareVisibility();
}

static void ApplyFixFlareArtifacts()
{
	if (!FixFlareArtifacts) return;

	g_Addresses.FlareVisibilityTexturePtr = GetAddress(Addr::FlareVisibilityTexturePtr);
	g_Addresses.TextureTablePtr = GetAddress(Addr::TextureTablePtr);
	TX_AllocTexture = reinterpret_cast<decltype(TX_AllocTexture)>(GetAddress(Addr::TX_AllocTexture));

	// The corona and the two glow paths set vfx_visibility as the vertex shader's modulate texture, read the copy's handle instead
	const Addr readers[] = { Addr::CoronaModulateTexture, Addr::FlareRenderBeginModulateTexture, Addr::FlareRenderModulateTexture };
	for (Addr reader : readers)
	{
		MemoryHelper::WriteMemory<uintptr_t>(GetAddress(reader), reinterpret_cast<uintptr_t>(&g_flareVisibilityCopy));
	}

	FlareOcclusionResolve = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FlareOcclusionResolve)), OnFlareOcclusionResolve);
	FlareOcclusionInitResolve = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FlareOcclusionInitResolve)), OnFlareOcclusionResolve);
	FlareOcclusionClearResolve = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FlareOcclusionClearResolve)), OnFlareOcclusionResolve);
}