#pragma once

#include "../../Globals.cpp"

// =========================
// PreloadStreamedTextures
// =========================

static std::mutex g_preloadMutex;
static std::deque<std::pair<IDirect3DBaseTexture9*, uint32_t>> g_preloadQueue;
static uint32_t g_preloadFrame = 0;
static bool g_isPreloadInstalled = false;

static void* g_CreateTexture = nullptr;
static void* g_CreateVolumeTexture = nullptr;
static void* g_CreateCubeTexture = nullptr;

static void QueueTexture(IDirect3DBaseTexture9* texture)
{
	std::lock_guard<std::mutex> lock(g_preloadMutex);
	if (g_preloadQueue.size() >= 8192) return;

	texture->AddRef();
	g_preloadQueue.emplace_back(texture, g_preloadFrame);
}

static HRESULT STDMETHODCALLTYPE CreateTexture_Hook(IDirect3DDevice9* pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
	HRESULT result = reinterpret_cast<decltype(&CreateTexture_Hook)>(g_CreateTexture)(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
	if (SUCCEEDED(result) && Pool == D3DPOOL_MANAGED) QueueTexture(*ppTexture);
	return result;
}

static HRESULT STDMETHODCALLTYPE CreateVolumeTexture_Hook(IDirect3DDevice9* pDevice, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
	HRESULT result = reinterpret_cast<decltype(&CreateVolumeTexture_Hook)>(g_CreateVolumeTexture)(pDevice, Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
	if (SUCCEEDED(result) && Pool == D3DPOOL_MANAGED) QueueTexture(*ppVolumeTexture);
	return result;
}

static HRESULT STDMETHODCALLTYPE CreateCubeTexture_Hook(IDirect3DDevice9* pDevice, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
	HRESULT result = reinterpret_cast<decltype(&CreateCubeTexture_Hook)>(g_CreateCubeTexture)(pDevice, EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
	if (SUCCEEDED(result) && Pool == D3DPOOL_MANAGED) QueueTexture(*ppCubeTexture);
	return result;
}

static void* ReplaceDeviceMethod(void** vtable, int index, void* hook)
{
	void* original = vtable[index];
	MemoryHelper::WriteMemory<uintptr_t>(reinterpret_cast<uintptr_t>(&vtable[index]), reinterpret_cast<uintptr_t>(hook));
	return original;
}

static void InstallTexturePreload()
{
	if (g_isPreloadInstalled || g_Addresses.DevicePtr == 0) return;

	IDirect3DDevice9* device = GetD3D9Device();
	if (!device) return;

	void** vtable = *reinterpret_cast<void***>(device);
	g_CreateTexture = ReplaceDeviceMethod(vtable, 23, &CreateTexture_Hook);
	g_CreateVolumeTexture = ReplaceDeviceMethod(vtable, 24, &CreateVolumeTexture_Hook);
	g_CreateCubeTexture = ReplaceDeviceMethod(vtable, 25, &CreateCubeTexture_Hook);
	g_isPreloadInstalled = true;
}

// Managed textures are uploaded on their first draw, the first frame of a streamed area used to upload hundreds of them
static void PreloadQueuedTextures()
{
	if (!g_isPreloadInstalled) return;

	g_preloadFrame++;

	LARGE_INTEGER start, now, frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);
	now = start;

	while (now.QuadPart - start.QuadPart < frequency.QuadPart / 500)
	{
		IDirect3DBaseTexture9* texture;
		{
			std::lock_guard<std::mutex> lock(g_preloadMutex);

			// A few frames for the game to fill the texture first
			if (g_preloadQueue.empty() || g_preloadFrame - g_preloadQueue.front().second < 3) break;

			texture = g_preloadQueue.front().first;
			g_preloadQueue.pop_front();
		}

		// Skip it when ours is the last reference
		ULONG references = texture->AddRef();
		texture->Release();

		if (references > 2)
		{
			texture->PreLoad();
		}

		texture->Release();
		QueryPerformanceCounter(&now);
	}
}
