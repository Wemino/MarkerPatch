#pragma once

#include "../../Globals.cpp"

static void ReleaseFlareFixResources()
{
	if (g_State.proxySurf)
	{
		g_State.proxySurf->Release();
		g_State.proxySurf = nullptr;
	}

	if (g_State.proxyTex)
	{
		g_State.proxyTex->Release();
		g_State.proxyTex = nullptr;
	}

	g_State.sourceTex = nullptr;

	g_State.resourcesValid = false;
	g_State.snapshotValid = false;
}

static bool TrackSourceTexture(IDirect3DBaseTexture9* src)
{
	IDirect3DDevice9* dev = GetD3D9Device();
	if (!src || !dev) return false;
	if (src->GetType() != D3DRTYPE_TEXTURE) return false;
	if (g_State.resourcesValid && g_State.sourceTex == src) return true;

	IDirect3DTexture9* src2d = static_cast<IDirect3DTexture9*>(src);
	D3DSURFACE_DESC desc{};
	if (FAILED(src2d->GetLevelDesc(0, &desc))) return false;

	ReleaseFlareFixResources();

	HRESULT hr = dev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &g_State.proxyTex, nullptr);
	if (FAILED(hr)) return false;

	hr = g_State.proxyTex->GetSurfaceLevel(0, &g_State.proxySurf);
	if (FAILED(hr))
	{
		ReleaseFlareFixResources(); return false;
	}

	g_State.sourceTex = src;
	g_State.resourcesValid = true;
	g_State.snapshotValid = false;
	return true;
}

// =========================
// FixFlareArtifacts
// =========================

static safetyhook::InlineHook AddCoronaModulatedQuad;
static uintptr_t AddCoronaModulatedQuad_Trampoline = 0;

static safetyhook::MidHook FlareSnapshot{};
static safetyhook::MidHook FlareTextureSubst{};
static safetyhook::MidHook DeviceCleanupPre{};

__declspec(naked) static int __cdecl AddCoronaModulatedQuad_Hook(DWORD* ci, const float* color, float x, float y, float z, float w, float angle, float sx, float sy, unsigned int pTexture, bool cutCorners)
{
	__asm
	{
		mov byte ptr[g_State.inFlareDraw], 1

		// Forward stack args, offset stays 0x28 as ESP drops
		push dword ptr[esp + 0x28] // cutCorners
		push dword ptr[esp + 0x28] // pTexture
		push dword ptr[esp + 0x28] // sy
		push dword ptr[esp + 0x28] // sx
		push dword ptr[esp + 0x28] // angle
		push dword ptr[esp + 0x28] // w
		push dword ptr[esp + 0x28] // z
		push dword ptr[esp + 0x28] // y
		push dword ptr[esp + 0x28] // x
		push dword ptr[esp + 0x28] // color

		// Call original function
		mov edx, [AddCoronaModulatedQuad_Trampoline]
		call edx

		// Balance stack
		add esp, 0x28

		mov byte ptr[g_State.inFlareDraw], 0
		ret
	}
}

static void OnFlareSnapshot(safetyhook::Context&)
{
	if (!g_State.resourcesValid || !g_State.sourceTex || !g_State.proxySurf) return;

	IDirect3DDevice9* dev = GetD3D9Device();
	if (!dev) return;

	IDirect3DTexture9* src2d = static_cast<IDirect3DTexture9*>(g_State.sourceTex);
	IDirect3DSurface9* srcSurf = nullptr;
	if (FAILED(src2d->GetSurfaceLevel(0, &srcSurf)) || !srcSurf) return;

	HRESULT hr = dev->StretchRect(srcSurf, nullptr, g_State.proxySurf, nullptr, D3DTEXF_NONE);
	srcSurf->Release();

	g_State.snapshotValid = SUCCEEDED(hr);
}

static void OnFlareTextureSubst(safetyhook::Context& ctx)
{
	if (ctx.ebx != 258) return;
	if (!g_State.inFlareDraw) return;

	IDirect3DBaseTexture9* tex = reinterpret_cast<IDirect3DBaseTexture9*>(ctx.ecx);
	if (!tex) return;

	if (!g_State.resourcesValid || g_State.sourceTex != tex)
	{
		TrackSourceTexture(tex);
	}

	if (!g_State.snapshotValid) return;
	if (g_State.sourceTex != tex) return;

	ctx.ecx = reinterpret_cast<uintptr_t>(g_State.proxyTex);
}

static void OnDeviceCleanupPre(safetyhook::Context&)
{
	ReleaseFlareFixResources();
	g_State.device = nullptr;
}

static void ApplyFixFlareArtifacts()
{
	if (!FixFlareArtifacts) return;

	AddCoronaModulatedQuad = HookHelper::CreateHook((void*)GetAddress(Addr::AddCoronaModulatedQuad), &AddCoronaModulatedQuad_Hook);
	AddCoronaModulatedQuad_Trampoline = AddCoronaModulatedQuad.trampoline().address();

	FlareSnapshot = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FlareSnapshot)), OnFlareSnapshot);
	FlareTextureSubst = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::FlareTextureSubst)), OnFlareTextureSubst);
	DeviceCleanupPre = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::DeviceCleanupPre)), OnDeviceCleanupPre);
}
