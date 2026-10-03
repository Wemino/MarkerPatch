#pragma once

#include "../../Globals.cpp"
#include "../../Shaders/FXAAPS.hpp"
#include "../../Shaders/SMAAShaders.hpp"
#include "../../Shaders/SMAATextures.hpp"
#include "../../Shaders/SupersamplingShaders.hpp"

// =========================
// ImprovedAntiAliasing
// =========================

enum AntiAliasingPass
{
	SMAA_EDGE_DETECTION,
	SMAA_BLENDING_WEIGHT,
	SMAA_NEIGHBORHOOD_BLENDING,
	SSAA_DOWNSAMPLE,
	AA_PASS_COUNT
};

static const unsigned char* const AA_VERTEX_SHADERS[AA_PASS_COUNT] = { g_SMAAEdgeDetectionVS, g_SMAABlendingWeightVS, g_SMAANeighborhoodBlendingVS, g_DownsampleVS };
static const unsigned char* const AA_PIXEL_SHADERS[AA_PASS_COUNT] = { g_SMAAEdgeDetectionPS, g_SMAABlendingWeightPS, g_SMAANeighborhoodBlendingPS, g_DownsamplePS };

static IDirect3DVertexShader9* g_aaVertexShaders[AA_PASS_COUNT] = {};
static IDirect3DPixelShader9* g_aaPixelShaders[AA_PASS_COUNT] = {};
static IDirect3DTexture9* g_smaaAreaTex = nullptr;
static IDirect3DTexture9* g_smaaSearchTex = nullptr;
static IDirect3DTexture9* g_smaaColorTex = nullptr;
static IDirect3DTexture9* g_smaaEdgesTex = nullptr;
static IDirect3DTexture9* g_smaaBlendTex = nullptr;

static safetyhook::InlineHook ScreenEdgeAA_RenderImmediate;
static safetyhook::InlineHook SetDisplayMode;

static safetyhook::MidHook ModeMatchStart{};
static safetyhook::MidHook ModeMatchEnd{};
static safetyhook::MidHook MenuCursorDelta{};
static safetyhook::MidHook ScreenEdgeAARender{};

// Supersampling renders the game at a multiple of the display resolution
static int ToRenderSize(int displaySize)
{
	return static_cast<int>(std::lround(displaySize * SSAAScale));
}

static int ToDisplaySize(int renderSize)
{
	return static_cast<int>(std::lround(renderSize / SSAAScale));
}

static void ReleaseTexture(IDirect3DTexture9*& tex)
{
	if (tex)
	{
		tex->Release();
		tex = nullptr;
	}
}

static void ReleaseSMAATargets()
{
	ReleaseTexture(g_smaaColorTex);
	ReleaseTexture(g_smaaEdgesTex);
	ReleaseTexture(g_smaaBlendTex);
}

static bool CreatePassShaders(IDirect3DDevice9* dev, AntiAliasingPass pass)
{
	if (!g_aaVertexShaders[pass] && FAILED(dev->CreateVertexShader(reinterpret_cast<const DWORD*>(AA_VERTEX_SHADERS[pass]), &g_aaVertexShaders[pass])))
		return false;

	if (!g_aaPixelShaders[pass] && FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(AA_PIXEL_SHADERS[pass]), &g_aaPixelShaders[pass])))
		return false;

	return true;
}

static IDirect3DTexture9* CreateLookupTexture(IDirect3DDevice9* dev, UINT width, UINT height, D3DFORMAT format, const unsigned char* data, UINT rowSize, UINT rowCount)
{
	IDirect3DTexture9* tex = nullptr;
	if (FAILED(dev->CreateTexture(width, height, 1, 0, format, D3DPOOL_MANAGED, &tex, nullptr))) return nullptr;

	D3DLOCKED_RECT locked{};
	if (FAILED(tex->LockRect(0, &locked, nullptr, 0)))
	{
		tex->Release();
		return nullptr;
	}

	for (UINT y = 0; y < height; y++)
	{
		BYTE* row = static_cast<BYTE*>(locked.pBits) + y * locked.Pitch;

		if (row)
		{
			if (y < rowCount)
			{
				memcpy(row, data + y * rowSize, rowSize);
			}
			else
			{
				memset(row, 0, rowSize);
			}
		}
	}

	tex->UnlockRect(0);
	return tex;
}

static bool CreateSMAAResources(IDirect3DDevice9* dev, const D3DSURFACE_DESC& desc)
{
	// Shaders and lookup textures survive device resets, the render targets follow the frame
	if (!CreatePassShaders(dev, SMAA_EDGE_DETECTION) ||
		!CreatePassShaders(dev, SMAA_BLENDING_WEIGHT) ||
		!CreatePassShaders(dev, SMAA_NEIGHBORHOOD_BLENDING)) {
		return false;
	}

	if (!g_smaaAreaTex)
	{
		g_smaaAreaTex = CreateLookupTexture(dev, SMAA_AREATEX_WIDTH, SMAA_AREATEX_HEIGHT, D3DFMT_A8L8, g_SMAAAreaTex, SMAA_AREATEX_WIDTH * 2, SMAA_AREATEX_ROWS);
	}

	if (!g_smaaSearchTex)
	{
		g_smaaSearchTex = CreateLookupTexture(dev, SMAA_SEARCHTEX_WIDTH, SMAA_SEARCHTEX_HEIGHT, D3DFMT_L8, g_SMAASearchTex, SMAA_SEARCHTEX_WIDTH, SMAA_SEARCHTEX_HEIGHT);
	}

	if (!g_smaaAreaTex || !g_smaaSearchTex) return false;

	if (g_smaaColorTex)
	{
		D3DSURFACE_DESC colorDesc{};
		g_smaaColorTex->GetLevelDesc(0, &colorDesc);

		if (colorDesc.Width != desc.Width || colorDesc.Height != desc.Height || colorDesc.Format != desc.Format)
		{
			ReleaseSMAATargets();
		}
	}

	if (!g_smaaColorTex)
	{
		dev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &g_smaaColorTex, nullptr);
		dev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_smaaEdgesTex, nullptr);
		dev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_smaaBlendTex, nullptr);
	}

	if (!g_smaaColorTex || !g_smaaEdgesTex || !g_smaaBlendTex)
	{
		ReleaseSMAATargets();
		return false;
	}

	return true;
}

static void SetFullscreenPassStates(IDirect3DDevice9* dev)
{
	dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
	dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
	dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	for (DWORD sampler = 0; sampler < 3; sampler++)
	{
		dev->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		dev->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		dev->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		dev->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		dev->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		dev->SetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, FALSE);
	}
}

static void DrawFullscreenPass(IDirect3DDevice9* dev, AntiAliasingPass pass, const D3DSURFACE_DESC& target)
{
	// Offset the quad by half a pixel so that pixels line up with texels
	float halfPixelX = 1.0f / target.Width;
	float halfPixelY = 1.0f / target.Height;

	const float quad[4][5] =
	{
		{ -1.0f - halfPixelX,  1.0f + halfPixelY, 0.5f, 0.0f, 0.0f },
		{  1.0f - halfPixelX,  1.0f + halfPixelY, 0.5f, 1.0f, 0.0f },
		{ -1.0f - halfPixelX, -1.0f + halfPixelY, 0.5f, 0.0f, 1.0f },
		{  1.0f - halfPixelX, -1.0f + halfPixelY, 0.5f, 1.0f, 1.0f }
	};

	dev->SetVertexShader(g_aaVertexShaders[pass]);
	dev->SetPixelShader(g_aaPixelShaders[pass]);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
}

static void RenderSMAA(IDirect3DDevice9* dev)
{
	IDirect3DSurface9* frameSurf = nullptr;
	if (FAILED(dev->GetRenderTarget(0, &frameSurf))) return;

	D3DSURFACE_DESC desc{};
	frameSurf->GetDesc(&desc);

	IDirect3DStateBlock9* stateBlock = nullptr;

	if (CreateSMAAResources(dev, desc) && SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL, &stateBlock)))
	{
		IDirect3DSurface9* colorSurf = nullptr;
		IDirect3DSurface9* edgesSurf = nullptr;
		IDirect3DSurface9* blendSurf = nullptr;
		g_smaaColorTex->GetSurfaceLevel(0, &colorSurf);
		g_smaaEdgesTex->GetSurfaceLevel(0, &edgesSurf);
		g_smaaBlendTex->GetSurfaceLevel(0, &blendSurf);

		// SMAA reads a copy of the frame and blends the result back into it
		dev->StretchRect(frameSurf, nullptr, colorSurf, nullptr, D3DTEXF_NONE);

		SetFullscreenPassStates(dev);

		const float rtMetrics[4] = { 1.0f / desc.Width, 1.0f / desc.Height, static_cast<float>(desc.Width), static_cast<float>(desc.Height) };
		dev->SetVertexShaderConstantF(0, rtMetrics, 1);
		dev->SetPixelShaderConstantF(0, rtMetrics, 1);

		dev->SetRenderTarget(0, edgesSurf);
		dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
		dev->SetTexture(0, g_smaaColorTex);
		DrawFullscreenPass(dev, SMAA_EDGE_DETECTION, desc);

		dev->SetRenderTarget(0, blendSurf);
		dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
		dev->SetTexture(0, g_smaaEdgesTex);
		dev->SetTexture(1, g_smaaAreaTex);
		dev->SetTexture(2, g_smaaSearchTex);
		dev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		dev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		DrawFullscreenPass(dev, SMAA_BLENDING_WEIGHT, desc);

		// Blend in linear space
		dev->SetRenderTarget(0, frameSurf);
		dev->SetTexture(0, g_smaaColorTex);
		dev->SetTexture(1, g_smaaBlendTex);
		dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		dev->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		DrawFullscreenPass(dev, SMAA_NEIGHBORHOOD_BLENDING, desc);

		stateBlock->Apply();
		stateBlock->Release();

		colorSurf->Release();
		edgesSurf->Release();
		blendSurf->Release();
	}

	frameSurf->Release();
}

static int __cdecl ScreenEdgeAA_RenderImmediate_Hook(int pRC)
{
	IDirect3DDevice9* dev = GetD3D9Device();

	if (dev)
	{
		RenderSMAA(dev);
	}

	return 0;
}

static HRESULT WINAPI CopyToBackBuffer_Hook(IDirect3DSurface9* backBuffer, const PALETTEENTRY* destPalette, const RECT* destRect, IDirect3DSurface9* frame, const PALETTEENTRY* srcPalette, const RECT* srcRect, DWORD filter, D3DCOLOR colorKey)
{
	IDirect3DDevice9* dev = nullptr;
	if (FAILED(frame->GetDevice(&dev))) return D3DERR_INVALIDCALL;

	D3DSURFACE_DESC frameDesc{};
	D3DSURFACE_DESC backBufferDesc{};
	frame->GetDesc(&frameDesc);
	backBuffer->GetDesc(&backBufferDesc);

	IDirect3DTexture9* frameTex = nullptr;
	IDirect3DSurface9* prevTarget = nullptr;
	IDirect3DStateBlock9* stateBlock = nullptr;
	HRESULT hr = D3DERR_INVALIDCALL;

	if (CreatePassShaders(dev, SSAA_DOWNSAMPLE) &&
		SUCCEEDED(frame->GetContainer(IID_PPV_ARGS(&frameTex))) &&
		SUCCEEDED(dev->GetRenderTarget(0, &prevTarget)) &&
		SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL, &stateBlock)))
	{
		// Average the render pixels covered by each display pixel, in linear space
		float scale = std::max(static_cast<float>(frameDesc.Width) / backBufferDesc.Width, static_cast<float>(frameDesc.Height) / backBufferDesc.Height);
		float tapCount = std::ceil(scale - 0.01f);
		const float tapParams[4] = { 1.0f / (tapCount * backBufferDesc.Width), 1.0f / (tapCount * backBufferDesc.Height), tapCount, 0.0f };

		SetFullscreenPassStates(dev);
		dev->SetRenderTarget(0, backBuffer);
		dev->SetTexture(0, frameTex);
		dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		dev->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		dev->SetPixelShaderConstantF(0, tapParams, 1);

		HRESULT sceneHr = dev->BeginScene();
		DrawFullscreenPass(dev, SSAA_DOWNSAMPLE, backBufferDesc);

		if (SUCCEEDED(sceneHr))
		{
			dev->EndScene();
		}

		dev->SetRenderTarget(0, prevTarget);
		stateBlock->Apply();
		hr = D3D_OK;
	}
	else
	{
		hr = dev->StretchRect(frame, nullptr, backBuffer, nullptr, D3DTEXF_LINEAR);
	}

	if (stateBlock) stateBlock->Release();
	if (prevTarget) prevTarget->Release();
	if (frameTex) frameTex->Release();
	dev->Release();

	return hr;
}

static char __cdecl SetDisplayMode_Hook(__int16 width, __int16 height, __int16 refreshRate, char fullscreen, char a5)
{
	return SetDisplayMode.ccall<char>(static_cast<__int16>(ToRenderSize(width)), static_cast<__int16>(ToRenderSize(height)), refreshRate, fullscreen, a5);
}

// Resolution requests are in render pixels, pick the display mode from the display resolution
static void OnModeMatchStart(safetyhook::Context& ctx)
{
	uint16_t* width = *reinterpret_cast<uint16_t**>(ctx.esp + 0x8);
	uint16_t* height = *reinterpret_cast<uint16_t**>(ctx.esp + 0xC);
	*width = static_cast<uint16_t>(ToDisplaySize(*width));
	*height = static_cast<uint16_t>(ToDisplaySize(*height));
}

// The back buffer keeps the display resolution while the game renders at the supersampled one
static void OnModeMatchEnd(safetyhook::Context& ctx)
{
	uint16_t* width = reinterpret_cast<uint16_t*>(ctx.ebx);
	uint16_t* height = reinterpret_cast<uint16_t*>(ctx.edi);
	*width = static_cast<uint16_t>(ToRenderSize(*width));
	*height = static_cast<uint16_t>(ToRenderSize(*height));
}

// The menu cursor moves by window pixels, which the game only converts to frame pixels in fullscreen
static void OnMenuCursorDelta(safetyhook::Context& ctx)
{
	if (!MemoryHelper::ReadMemory<BOOL>(g_Addresses.PresentParamsPtr + 0x20)) return;

	float* delta = reinterpret_cast<float*>(ctx.esp + 0xC);
	delta[0] *= SSAAScale;
	delta[1] *= SSAAScale;
}

// The pass only runs when a level effect requests it, request it every frame instead
static void OnScreenEdgeAARender(safetyhook::Context&)
{
	*reinterpret_cast<DWORD*>(g_Addresses.AAValsFlagsPtr) |= 0x400000;
}

static void ApplySupersampling()
{
	if (SSAAScale == 1.0f) return;

	DWORD addr_ModeMatch = ScanModuleSignature(g_State.GameModule, "81 EC 74 01 00 00 80 BC 24 88 01 00 00 00 53 8B 9C 24 80 01 00 00", "ModeMatch");
	DWORD addr_SetDisplayMode = ScanModuleSignature(g_State.GameModule, "88 51 11 0F B7 51 08 50 0F B7 41 04 52 50 E8", "SetDisplayMode");
	DWORD addr_CreateDevice = ScanModuleSignature(g_State.GameModule, "BE ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 18 84 C0 75 05 E8", "CreateDevice");
	DWORD addr_WindowResize = ScanModuleSignature(g_State.GameModule, "0F B7 05 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 99 F7 F9 66 0F EF C0", "WindowResize");
	DWORD addr_CursorUpdate = ScanModuleSignature(g_State.GameModule, "66 8B 35 ?? ?? ?? ?? 66 8B 3D ?? ?? ?? ?? 85 C9 7C 16", "CursorUpdate");
	DWORD addr_CursorRestore = ScanModuleSignature(g_State.GameModule, "0F B7 05 ?? ?? ?? ?? 0F B7 0D ?? ?? ?? ?? 48 49 F3 0F 2A C0", "CursorRestore");
	DWORD addr_CursorClip = ScanModuleSignature(g_State.GameModule, "0F B7 05 ?? ?? ?? ?? 0F B7 0D ?? ?? ?? ?? 89 44 24 10 99", "CursorClip");
	DWORD addr_DisplayModeGetters = ScanModuleSignature(g_State.GameModule, "66 A1 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC 66 A1 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC 66 A1 ?? ?? ?? ?? C3", "DisplayModeGetters");
	DWORD addr_MenuCursorDelta = ScanModuleSignature(g_State.GameModule, "0F B7 0D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? F3 0F 10 44 24 18 F3 0F 58 44 24 0C", "MenuCursorDelta");
	DWORD addr_CopyToBackBuffer = ScanModuleSignature(g_State.GameModule, "8B 4C 24 04 8B 14 24 6A 00 6A 01 6A 00 6A 00 51 6A 00 6A 00 52 E8", "CopyToBackBuffer");

	if (addr_ModeMatch == 0 ||
		addr_SetDisplayMode == 0 ||
		addr_CreateDevice == 0 ||
		addr_WindowResize == 0 ||
		addr_CursorUpdate == 0 ||
		addr_CursorRestore == 0 ||
		addr_CursorClip == 0 ||
		addr_DisplayModeGetters == 0 ||
		addr_MenuCursorDelta == 0 ||
		addr_CopyToBackBuffer == 0) {
		return;
	}

	g_Addresses.PresentParamsPtr = MemoryHelper::ReadMemory<int>(addr_CreateDevice + 0x1);

	ModeMatchStart = safetyhook::create_mid(reinterpret_cast<void*>(addr_ModeMatch), OnModeMatchStart);
	ModeMatchEnd = safetyhook::create_mid(reinterpret_cast<void*>(addr_ModeMatch + 0x199), OnModeMatchEnd);

	DWORD addr_SetDisplayModeThunk = MemoryHelper::ResolveRelativeAddress(addr_SetDisplayMode, 0xF);
	SetDisplayMode = HookHelper::CreateHook((void*)addr_SetDisplayModeThunk, &SetDisplayMode_Hook);

	// The window, cursor and settings menu code expects the back buffer to be as large as the frame, use the back buffer size instead
	DWORD backBufferWidthPtr = g_Addresses.PresentParamsPtr;
	DWORD backBufferHeightPtr = g_Addresses.PresentParamsPtr + 0x4;

	MemoryHelper::WriteMemory<DWORD>(addr_WindowResize + 0x3, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_WindowResize + 0x2D, backBufferHeightPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorUpdate + 0x3, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorUpdate + 0xA, backBufferHeightPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorUpdate + 0x7C, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorUpdate + 0x83, backBufferHeightPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorRestore + 0x3, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorRestore + 0xA, backBufferHeightPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorClip + 0x3, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_CursorClip + 0xA, backBufferHeightPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_DisplayModeGetters + 0x12, backBufferWidthPtr);
	MemoryHelper::WriteMemory<DWORD>(addr_DisplayModeGetters + 0x22, backBufferHeightPtr);

	MenuCursorDelta = safetyhook::create_mid(reinterpret_cast<void*>(addr_MenuCursorDelta), OnMenuCursorDelta);

	// Downsample the frame into the back buffer instead of copying it
	MemoryHelper::MakeCALL(addr_CopyToBackBuffer + 0x15, reinterpret_cast<uintptr_t>(&CopyToBackBuffer_Hook));

	g_State.isSupersampling = true;
}

static void ApplyImprovedAntiAliasing()
{
	if (ImprovedAntiAliasingMode == AA_DISABLED) return;

	if (ImprovedAntiAliasingMode == AA_SSAA)
	{
		ApplySupersampling();
		return;
	}

	DWORD addr_AlchemyEdgeAAShader_Load = ScanModuleSignature(g_State.GameModule, "A1 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 56 50 8B F1 8B 0D ?? ?? ?? ?? 6A 10 51 68 ?? ?? ?? ?? 52 8B CE E8 ?? ?? ?? ?? 8B 46 14 89 86 AC 00 00 00 8B 0D", "AlchemyEdgeAAShader_Load");
	DWORD addr_ScreenEdgeAA_Render = ScanModuleSignature(g_State.GameModule, "F3 0F 10 05 ?? ?? ?? ?? 0F 2F 05 ?? ?? ?? ?? 77 09 0F 2F 05 ?? ?? ?? ?? 76 0C F7 05 ?? ?? ?? ?? 00 00 40 00 74", "ScreenEdgeAA_Render");

	if (addr_AlchemyEdgeAAShader_Load == 0 ||
		addr_ScreenEdgeAA_Render == 0) {
		return;
	}

	if (ImprovedAntiAliasingMode == AA_FXAA)
	{
		// Replace the depth and normal based edge blur with FXAA
		DWORD addr_PSTable = MemoryHelper::ReadMemory<DWORD>(MemoryHelper::ReadMemory<DWORD>(addr_AlchemyEdgeAAShader_Load + 0x30));
		uint32_t fxaaPSPtr = (uint32_t)(uintptr_t)g_FXAAPS;

		MemoryHelper::WriteMemory<uint32_t>(addr_PSTable, fxaaPSPtr);
		MemoryHelper::WriteMemory<uint32_t>(addr_PSTable + 0x4, fxaaPSPtr);
		MemoryHelper::WriteMemory<uint32_t>(addr_PSTable + 0x8, fxaaPSPtr);
		MemoryHelper::WriteMemory<uint32_t>(addr_PSTable + 0xC, fxaaPSPtr);
	}
	else
	{
		// Replace the whole edge blur pass with SMAA
		DWORD addr_ScreenEdgeAA_RenderImmediate = MemoryHelper::ResolveRelativeAddress(addr_ScreenEdgeAA_Render, 0x2C);
		ScreenEdgeAA_RenderImmediate = HookHelper::CreateHook((void*)addr_ScreenEdgeAA_RenderImmediate, &ScreenEdgeAA_RenderImmediate_Hook);
	}

	g_Addresses.AAValsFlagsPtr = MemoryHelper::ReadMemory<int>(addr_ScreenEdgeAA_Render + 0x1C);

	ScreenEdgeAARender = safetyhook::create_mid(reinterpret_cast<void*>(addr_ScreenEdgeAA_Render + 0x1A), OnScreenEdgeAARender);
}
