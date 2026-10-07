#pragma once

#include "../../Globals.cpp"

// =========================
// FixClothPhysics
// =========================

safetyhook::InlineHook VerletIntegrate;
safetyhook::InlineHook ClothComponent_ApplyWindForce;
safetyhook::InlineHook ClothComponent_Teleport;
safetyhook::InlineHook ClothRelaxation;
safetyhook::InlineHook CapeRelaxation;

// ClothComponent
static constexpr uintptr_t OFF_CLOTH_PARAMS = 0x1C;
static constexpr uintptr_t OFF_CLOTH_WIND_FORCE = 0x60;
static constexpr uintptr_t OFF_CLOTH_WIND_UPDATE = 0x70;

// ClothParams
static constexpr uintptr_t OFF_PARAMS_ELAPSED_TIME = 0xB4;
static constexpr uintptr_t OFF_PARAMS_TIME_SCALE = 0xB8;
static constexpr uintptr_t OFF_PARAMS_POSITION_COUNT = 0xBC;
static constexpr uintptr_t OFF_PARAMS_OLD_POSITIONS = 0xC8;
static constexpr uintptr_t OFF_PARAMS_POSITIONS = 0xCC;
static constexpr uintptr_t OFF_PARAMS_FORCES = 0xD4;
static constexpr uintptr_t OFF_PARAMS_RIGID = 0xE0;
static constexpr uintptr_t OFF_PARAMS_DAMPING = 0x134;

// CapeParams
static constexpr uintptr_t OFF_CAPE_PARAMS_STRICT_TOP_TO_BOTTOM_PULL = 0x178;

static constexpr float CLOTH_PRECISION_STEPS = 12.0f;
static constexpr float CLOTH_GRAVITY = 9.81f;

static constexpr float CLOTH_RESET_DISTANCE = 0.5f;

static constexpr float CLOTH_DELAY_RISE = 0.25f;
static constexpr float CLOTH_DELAY_FALL = 0.05f;

struct ClothPoint
{
	float x, y, z, w;
};

struct ClothState
{
	std::vector<ClothPoint> positions;
	std::vector<ClothPoint> oldPositions;
	std::vector<ClothPoint> offsets[2];
	std::vector<ClothPoint> shownOldPositions;
	ClothPoint stepRoot{};
	ClothPoint translation{};
	float stepTime = 0.0f;
	float lastStep = 0.0f;
	float offsetStep = 0.0f;
	float delay = 0.0f;
	uint32_t offsetCount = 0;
	ULONGLONG lastUse = 0;
	bool isStepping = false;
	bool isShowing = false;
	bool isRelaxSkipped = false;
	bool isTranslated = false;
};

static constexpr float CLOTH_WIND_FADE = 0.75f;

static float g_ragdollMaxInvFrameTime = 1000.0f;
static std::unordered_map<uintptr_t, float> g_clothWindTimes;
static std::mutex g_clothStateMutex;
static std::unordered_map<uintptr_t, ClothState> g_clothStates;
static thread_local bool t_isInCapeRelaxation = false;

static float GetClothMinStep(const ClothPoint& root)
{
	int exponent = 0;
	(void)frexpf(std::max({ fabsf(root.x), fabsf(root.y), fabsf(root.z) }), &exponent);
	float precision = ldexpf(1.0f, exponent - 24);

	return std::min(sqrtf(CLOTH_PRECISION_STEPS * precision / CLOTH_GRAVITY), TARGET_FRAME_TIME);
}

static bool IsClothValid(const ClothPoint* positions, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
	{
		if (!std::isfinite(positions[i].x) || !std::isfinite(positions[i].y) || !std::isfinite(positions[i].z)) return false;
	}

	return true;
}

static bool GetClothRoot(const ClothPoint* positions, const float* rigid, uint32_t count, ClothPoint& root)
{
	root = {};
	uint32_t pinned = 0;

	for (uint32_t i = 0; i < count; i++)
	{
		if (rigid[i] != 0.0f) continue;

		root.x += positions[i].x;
		root.y += positions[i].y;
		root.z += positions[i].z;
		pinned++;
	}

	if (pinned == 0) return false;

	root.x /= pinned;
	root.y /= pinned;
	root.z /= pinned;
	return true;
}

static ClothState& GetClothState(uintptr_t pParams, uint32_t count)
{
	std::lock_guard<std::mutex> lock(g_clothStateMutex);
	ULONGLONG now = GetTickCount64();

	if (g_clothStates.size() > 64)
	{
		std::erase_if(g_clothStates, [now](const auto& entry) { return now - entry.second.lastUse > 30000; });
	}

	ClothState& state = g_clothStates[pParams];
	state.lastUse = now;

	if (state.positions.size() != count)
	{
		state = {};
		state.lastUse = now;
		state.positions.resize(count);
		state.oldPositions.resize(count);
		state.offsets[0].resize(count);
		state.offsets[1].resize(count);
		state.shownOldPositions.resize(count);
	}

	if (state.isTranslated)
	{
		const ClothPoint& translation = state.translation;

		for (auto* points : { &state.positions, &state.oldPositions, &state.shownOldPositions })
		{
			for (ClothPoint& point : *points)
			{
				point = { point.x + translation.x, point.y + translation.y, point.z + translation.z, point.w + translation.w };
			}
		}

		state.stepRoot = { state.stepRoot.x + translation.x, state.stepRoot.y + translation.y, state.stepRoot.z + translation.z, state.stepRoot.w };
		state.translation = {};
		state.isTranslated = false;
	}

	return state;
}

static bool IsClothReset(const ClothState& state, const ClothPoint* oldPositions, const float* rigid, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++)
	{
		if (rigid[i] == 0.0f) continue;

		float x = oldPositions[i].x - state.shownOldPositions[i].x;
		float y = oldPositions[i].y - state.shownOldPositions[i].y;
		float z = oldPositions[i].z - state.shownOldPositions[i].z;

		if (!(x * x + y * y + z * z <= CLOTH_RESET_DISTANCE * CLOTH_RESET_DISTANCE)) return true;
	}

	return false;
}

static ClothState* FindClothState(uintptr_t pParams)
{
	std::lock_guard<std::mutex> lock(g_clothStateMutex);
	auto it = g_clothStates.find(pParams);
	return it != g_clothStates.end() ? &it->second : nullptr;
}

static void CaptureCloth(ClothState& state, const ClothPoint* positions, const ClothPoint* oldPositions, uint32_t count)
{
	memcpy(state.positions.data(), positions, count * sizeof(ClothPoint));
	memcpy(state.oldPositions.data(), oldPositions, count * sizeof(ClothPoint));
	memcpy(state.shownOldPositions.data(), oldPositions, count * sizeof(ClothPoint));
	std::swap(state.offsets[0], state.offsets[1]);

	for (uint32_t i = 0; i < count; i++)
	{
		state.offsets[0][i] = { positions[i].x - state.stepRoot.x, positions[i].y - state.stepRoot.y, positions[i].z - state.stepRoot.z, 0.0f };
	}

	state.offsetStep = state.lastStep;
	state.offsetCount = std::min(state.offsetCount + 1, 2u);
	state.isStepping = false;
}

static void ShowCloth(ClothState& state, ClothPoint* positions, ClothPoint* oldPositions, const float* rigid, uint32_t count, const ClothPoint& root)
{
	float time = state.stepTime - state.delay; // From its last step
	float progress = 1.0f;

	if (state.offsetCount >= 2 && time < 0.0f && state.offsetStep > 0.0f)
	{
		progress = std::max(1.0f + time / state.offsetStep, 0.0f);
	}

	for (uint32_t i = 0; i < count; i++)
	{
		if (rigid[i] == 0.0f) continue;

		const ClothPoint& offset = state.offsets[0][i];
		const ClothPoint& prevOffset = state.offsetCount >= 2 ? state.offsets[1][i] : offset;
		ClothPoint shown = { root.x + prevOffset.x + (offset.x - prevOffset.x) * progress, root.y + prevOffset.y + (offset.y - prevOffset.y) * progress, root.z + prevOffset.z + (offset.z - prevOffset.z) * progress, state.positions[i].w };

		oldPositions[i] = { state.oldPositions[i].x + shown.x - state.positions[i].x, state.oldPositions[i].y + shown.y - state.positions[i].y, state.oldPositions[i].z + shown.z - state.positions[i].z, state.oldPositions[i].w };
		positions[i] = shown;
		state.shownOldPositions[i] = oldPositions[i];
	}

	state.isShowing = true;
}

static void __cdecl VerletIntegrate_Hook(uintptr_t pParams)
{
	float& elapsedTime = *reinterpret_cast<float*>(pParams + OFF_PARAMS_ELAPSED_TIME);
	float& timeScale = *reinterpret_cast<float*>(pParams + OFF_PARAMS_TIME_SCALE);
	float& damping = *reinterpret_cast<float*>(pParams + OFF_PARAMS_DAMPING);
	uint32_t count = *reinterpret_cast<uint32_t*>(pParams + OFF_PARAMS_POSITION_COUNT);
	ClothPoint* positions = *reinterpret_cast<ClothPoint**>(pParams + OFF_PARAMS_POSITIONS);
	ClothPoint* oldPositions = *reinterpret_cast<ClothPoint**>(pParams + OFF_PARAMS_OLD_POSITIONS);
	ClothPoint* forces = *reinterpret_cast<ClothPoint**>(pParams + OFF_PARAMS_FORCES);
	const float* rigid = *reinterpret_cast<const float**>(pParams + OFF_PARAMS_RIGID);

	float authoredDamping = damping;
	float frameTime = elapsedTime;
	float frameTimeScale = timeScale;

	if (!(frameTime > 0.0f) || count == 0 || positions == nullptr || oldPositions == nullptr || forces == nullptr || rigid == nullptr || !IsClothValid(positions, count))
	{
		if (ClothState* state = FindClothState(pParams))
		{
			state->isRelaxSkipped = false;
		}

		VerletIntegrate.unsafe_ccall<void>(pParams);
		return;
	}

	ClothState& state = GetClothState(pParams, count);
	ClothPoint root{};
	bool hasRoot = GetClothRoot(positions, rigid, count, root);

	if (state.isStepping)
	{
		CaptureCloth(state, positions, oldPositions, count);
		state.isShowing = false;
	}

	if (state.offsetCount > 0 && IsClothReset(state, oldPositions, rigid, count))
	{
		state.isShowing = false;
		state.offsetCount = 0;
		state.lastStep = 0.0f;
		state.stepTime = 0.0f;
	}

	state.stepTime += frameTime;
	float minStep = hasRoot ? GetClothMinStep(root) : 0.0f;

	bool isFrameShort = frameTime < minStep;
	float delayChange = frameTime * (isFrameShort ? CLOTH_DELAY_RISE : CLOTH_DELAY_FALL);
	state.delay = std::clamp(isFrameShort ? minStep : 0.0f, state.delay - delayChange, state.delay + delayChange);

	if (state.stepTime >= minStep || state.lastStep <= 0.0f || state.offsetCount == 0)
	{
		if (state.isShowing)
		{
			for (uint32_t i = 0; i < count; i++)
			{
				if (rigid[i] == 0.0f) continue;

				positions[i] = state.positions[i];
				oldPositions[i] = state.oldPositions[i];
			}

			state.isShowing = false;
		}

		float step = state.lastStep > 0.0f ? state.stepTime : frameTime;
		elapsedTime = step;
		timeScale = state.lastStep > 0.0f ? step / state.lastStep : frameTimeScale;

		if (authoredDamping > 0.0f)
		{
			damping = powf(authoredDamping, step / TARGET_FRAME_TIME);
		}

		VerletIntegrate.unsafe_ccall<void>(pParams);

		elapsedTime = frameTime;
		timeScale = frameTimeScale;
		damping = authoredDamping;

		state.lastStep = step;
		state.stepTime = 0.0f;
		state.stepRoot = root;
		state.isStepping = hasRoot;
		state.isRelaxSkipped = false;
		return;
	}

	memset(forces, 0, count * sizeof(ClothPoint));

	ShowCloth(state, positions, oldPositions, rigid, count, root);
	state.isRelaxSkipped = true;
}

static void RelaxCloth(safetyhook::InlineHook& hook, uintptr_t pParams)
{
	ClothState* state = FindClothState(pParams);

	if (state == nullptr || !(state->isStepping || state->isRelaxSkipped))
	{
		hook.unsafe_ccall<void>(pParams);
		return;
	}

	if (!state->isStepping) return;

	hook.unsafe_ccall<void>(pParams);

	uint32_t count = *reinterpret_cast<uint32_t*>(pParams + OFF_PARAMS_POSITION_COUNT);
	ClothPoint* positions = *reinterpret_cast<ClothPoint**>(pParams + OFF_PARAMS_POSITIONS);
	ClothPoint* oldPositions = *reinterpret_cast<ClothPoint**>(pParams + OFF_PARAMS_OLD_POSITIONS);
	const float* rigid = *reinterpret_cast<const float**>(pParams + OFF_PARAMS_RIGID);

	CaptureCloth(*state, positions, oldPositions, count);

	if (state->delay > 0.0f && state->offsetCount >= 2)
	{
		ShowCloth(*state, positions, oldPositions, rigid, count, state->stepRoot);
	}
}

static void __cdecl ClothRelaxation_Hook(uintptr_t pParams)
{
	if (t_isInCapeRelaxation)
	{
		ClothRelaxation.unsafe_ccall<void>(pParams);
		return;
	}

	RelaxCloth(ClothRelaxation, pParams);
}

static void __cdecl CapeRelaxation_Hook(uintptr_t pParams)
{
	if (*reinterpret_cast<uint8_t*>(pParams + OFF_CAPE_PARAMS_STRICT_TOP_TO_BOTTOM_PULL) == 0)
	{
		CapeRelaxation.unsafe_ccall<void>(pParams);
		return;
	}

	t_isInCapeRelaxation = true;
	RelaxCloth(CapeRelaxation, pParams);
	t_isInCapeRelaxation = false;
}

static void __fastcall ClothComponent_ApplyWindForce_Hook(uintptr_t thisp)
{
	uintptr_t pParams = *reinterpret_cast<uintptr_t*>(thisp + OFF_CLOTH_PARAMS);

	if (pParams == 0)
	{
		ClothComponent_ApplyWindForce.unsafe_thiscall<void>(thisp);
		return;
	}

	if (g_clothWindTimes.size() > 1024) g_clothWindTimes.clear();

	float& windTime = g_clothWindTimes[thisp];
	bool isStep = windTime <= 0.0f;

	if (isStep)
	{
		windTime += TARGET_FRAME_TIME;
	}

	windTime = std::max(windTime - *reinterpret_cast<float*>(pParams + OFF_PARAMS_ELAPSED_TIME), -TARGET_FRAME_TIME);

	if (isStep)
	{
		ClothComponent_ApplyWindForce.unsafe_thiscall<void>(thisp);
		return;
	}

	float* wind = reinterpret_cast<float*>(thisp + OFF_CLOTH_WIND_FORCE);
	int& windSteps = *reinterpret_cast<int*>(thisp + OFF_CLOTH_WIND_UPDATE);

	float savedWind[4];
	memcpy(savedWind, wind, sizeof(savedWind));
	int savedWindSteps = windSteps;

	for (int i = 0; i < 4; i++)
	{
		wind[i] = savedWind[i] / CLOTH_WIND_FADE;
	}

	windSteps = std::max(savedWindSteps, 1);

	ClothComponent_ApplyWindForce.unsafe_thiscall<void>(thisp);

	memcpy(wind, savedWind, sizeof(savedWind));
	windSteps = savedWindSteps;
}

static void __fastcall ClothComponent_Teleport_Hook(uintptr_t thisp, void*, const ClothPoint* offset)
{
	ClothComponent_Teleport.unsafe_thiscall<void>(thisp, offset);

	uintptr_t pParams = *reinterpret_cast<uintptr_t*>(thisp + OFF_CLOTH_PARAMS);
	if (pParams == 0) return;

	std::lock_guard<std::mutex> lock(g_clothStateMutex);
	auto it = g_clothStates.find(pParams);
	if (it == g_clothStates.end()) return;

	ClothState& state = it->second;
	state.translation = { state.translation.x + offset->x, state.translation.y + offset->y, state.translation.z + offset->z, state.translation.w + offset->w };
	state.isTranslated = true;
}

static void ApplyFixClothPhysics()
{
	if (!FixClothPhysics) return;

	DWORD addr_VerletIntegrate = GetAddress(Addr::VerletIntegrate);
	DWORD addr_ClothComponent_ApplyWindForce = GetAddress(Addr::ClothComponent_ApplyWindForce);
	DWORD addr_ClothComponent_Teleport = GetAddress(Addr::ClothComponent_Teleport);
	DWORD addr_ClothRelaxation = GetAddress(Addr::ClothRelaxation);
	DWORD addr_CapeRelaxation = GetAddress(Addr::CapeRelaxation);
	DWORD addr_RagdollComponent_DriveRigidBodies = GetAddress(Addr::RagdollComponent_DriveRigidBodies);

	MemoryHelper::WriteMemory<uintptr_t>(addr_RagdollComponent_DriveRigidBodies + 0x4, reinterpret_cast<uintptr_t>(&g_ragdollMaxInvFrameTime));

	VerletIntegrate = HookHelper::CreateHook((void*)addr_VerletIntegrate, &VerletIntegrate_Hook);
	ClothComponent_Teleport = HookHelper::CreateHook((void*)addr_ClothComponent_Teleport, &ClothComponent_Teleport_Hook);
	ClothRelaxation = HookHelper::CreateHook((void*)addr_ClothRelaxation, &ClothRelaxation_Hook);
	CapeRelaxation = HookHelper::CreateHook((void*)addr_CapeRelaxation, &CapeRelaxation_Hook);
	ClothComponent_ApplyWindForce = HookHelper::CreateHook((void*)addr_ClothComponent_ApplyWindForce, &ClothComponent_ApplyWindForce_Hook);
}
