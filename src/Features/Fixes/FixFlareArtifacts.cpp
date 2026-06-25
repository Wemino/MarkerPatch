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

static safetyhook::InlineHook RenderFlare;
static uintptr_t RenderFlare_Trampoline = 0;

__declspec(naked) static int __cdecl RenderFlare_Hook(DWORD* pFlare, int textureId, float posX, float posY, float sizeX, float sizeY, float rotation, float alpha, float colorReg, int renderPass, bool isScreenSpace)
{
	__asm
	{
		mov byte ptr[g_State.inFlareDraw], 1

		// Forward stack args, offset stays 0x28 as ESP drops
		push dword ptr[esp + 0x28] // isScreenSpace
		push dword ptr[esp + 0x28] // renderPass
		push dword ptr[esp + 0x28] // colorReg
		push dword ptr[esp + 0x28] // alpha
		push dword ptr[esp + 0x28] // rotation
		push dword ptr[esp + 0x28] // sizeY
		push dword ptr[esp + 0x28] // sizeX
		push dword ptr[esp + 0x28] // posY
		push dword ptr[esp + 0x28] // posX
		push dword ptr[esp + 0x28] // textureId

		// Call original function
		mov edx, [RenderFlare_Trampoline]
		call edx

		// Balance stack
		add esp, 0x28

		mov byte ptr[g_State.inFlareDraw], 0
		ret
	}
}
static void ApplyFixFlareArtifacts()
{
	if (!FixFlareArtifacts) return;

	DWORD addr_RenderFlare = ScanModuleSignature(g_State.GameModule, "56 8B F0 8B 06 57 BF 01 00 00 00 23 C7", "RenderFlare");
	DWORD addr_FlareSnapshot = ScanModuleSignature(g_State.GameModule, "83 C0 28 88 4E 1E 2B D5 8B 36 3B D3 0F 85", "FlareSnapshot");
	DWORD addr_FlareTextureSubst = ScanModuleSignature(g_State.GameModule, "8B 10 51 6A 04 53 50 8B 82 14 01 00 00 FF D0", "FlareTextureSubst");
	DWORD addr_DeviceCleanupPre = ScanModuleSignature(g_State.GameModule, "85 C0 74 12 8B 08 8B 51 08 50 FF D2 C7 05 ?? ?? ?? ?? ?? ?? ?? ?? E9", "DeviceCleanupPre");

	if (addr_RenderFlare == 0 ||
		addr_FlareSnapshot == 0 ||
		addr_FlareTextureSubst == 0 ||
		addr_DeviceCleanupPre == 0) {
		return;
	}

	RenderFlare = HookHelper::CreateHook((void*)addr_RenderFlare, &RenderFlare_Hook);
	RenderFlare_Trampoline = RenderFlare.trampoline().address();

	static SafetyHookMid FlareSnapshot{};
	FlareSnapshot = safetyhook::create_mid(reinterpret_cast<void*>(addr_FlareSnapshot + 0x2B),
		[](safetyhook::Context&)
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
	);

	static SafetyHookMid FlareTextureSubst{};
	FlareTextureSubst = safetyhook::create_mid(reinterpret_cast<void*>(addr_FlareTextureSubst + 0x1B),
		[](safetyhook::Context& ctx)
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
	);

	static SafetyHookMid DeviceCleanupPre{};
	DeviceCleanupPre = safetyhook::create_mid(reinterpret_cast<void*>(addr_DeviceCleanupPre - 0x3B),
		[](safetyhook::Context&)
		{
			ReleaseFlareFixResources();
			g_State.device = nullptr;
		}
	);
}
