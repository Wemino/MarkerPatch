#pragma once

#include "../../Globals.cpp"

// =====================
// RawMouseInput
// =====================

safetyhook::InlineHook ApplyControlConfiguration;
safetyhook::InlineHook UpdateMenuCursor;
safetyhook::InlineHook RE4ChaseCamera_Update;
safetyhook::InlineHook OrbitCamera_Update;
safetyhook::InlineHook PlayerZGJumpSM_ProcessAimingControls;
safetyhook::InlineHook hkGetRawInputData;

static safetyhook::MidHook ZeroGravityRotation{};
static safetyhook::MidHook RE4ChaseCameraAim{};
static safetyhook::MidHook GroundAim{};
static safetyhook::MidHook GroundAimPitch{};
static safetyhook::MidHook DraggedAim{};
static safetyhook::MidHook StationaryShootingAim{};
static safetyhook::MidHook DecompressionAim{};
static safetyhook::MidHook HangingAim{};

static float(__cdecl* SensitivityInterp)(float, float, float) = nullptr;
static float(__cdecl* PlayerSpeedSettings_GetGunModifier)() = nullptr;

// Arguments of an AimingData::UpdateAim call
struct AimUpdate
{
	float deltaPitch;
	float deltaYaw;
	float deltaRoll;
	int type;
};

static void GetMouseRotation(float divisor, float& pitch, float& yaw)
{
	yaw = (g_State.frameRawX * g_State.mouseSens) / -divisor;
	pitch = (g_State.frameRawY * g_State.mouseSens) / divisor;

	if (g_State.isXInverted) yaw = -yaw;
	if (g_State.isYInverted) pitch = -pitch;
}

static bool GetAimRotation(float gamePitch, float gameYaw, float& pitch, float& yaw)
{
	if (g_State.isControllerActive)
	{
		if (g_State.frameGyroPitch == 0.0f && g_State.frameGyroYaw == 0.0f) return false;

		pitch = gamePitch + g_State.frameGyroPitch;
		yaw = gameYaw + g_State.frameGyroYaw;
		return true;
	}

	GetMouseRotation(1500.0f, pitch, yaw);
	return true;
}

static float GetAimSpeedModifier(DWORD setting)
{
	DWORD address = g_Addresses.PlayerSpeedSettingsPtr + setting;
	float speed = MemoryHelper::ReadMemory<float>(address);
	float modifier = SensitivityInterp(MemoryHelper::ReadMemory<float>(address + 72), speed, MemoryHelper::ReadMemory<float>(address + 108)) / speed;
	return modifier * PlayerSpeedSettings_GetGunModifier();
}

static void RecordMouseAim(uintptr_t aim, float pitchDelta, float yawDelta)
{
	if (g_State.isControllerActive) return;

	if (g_State.mouseAimData != aim)
	{
		g_State.mouseAimData = aim;
		g_State.mouseAimPitch = 0.0f;
		g_State.mouseAimYaw = 0.0f;
	}

	g_State.mouseAimPitch += pitchDelta;
	g_State.mouseAimYaw += yawDelta;
}

static int __stdcall ApplyControlConfiguration_Hook(int a1)
{
	// Get the current mouse sensitivity
	g_State.isXInverted = *(BYTE*)(a1);
	g_State.isYInverted = *(BYTE*)(a1 + 1);
	g_State.mouseSens = *(float*)(a1 + 12) * 1.8f + 0.1f;
	ControllerHelper::SetGyroInvertX(g_State.isXInverted);
	ControllerHelper::SetGyroInvertY(g_State.isYInverted);
	return ApplyControlConfiguration.stdcall<int>(a1);
}

static void __fastcall UpdateMenuCursor_Hook(int thisp, float a2)
{
	// Get input device manager instance
	int inputDeviceManager = *(int*)g_Addresses.InputDeviceManagerPtr;

	// Check if controller is being used (18 = mouse)
	g_State.isControllerActive = (*(int*)(inputDeviceManager + 1400) != 18);

	UpdateMenuCursor.unsafe_fastcall<void>(thisp, a2);
}

// =====================
// Cameras
// =====================

static int __fastcall RE4ChaseCamera_Update_Hook(int thisp, float frameTime)
{
	if (!g_State.isControllerActive)
	{
		// Framerate independant sensitivity
		frameTime = 1.0f;
	}

	return RE4ChaseCamera_Update.unsafe_fastcall<int>(thisp, frameTime);
}

static int __fastcall OrbitCamera_Update_Hook(int thisp, float frameTime)
{
	if (!g_State.isControllerActive)
	{
		int targetPlayer = *(int*)(thisp + 124);
		uintptr_t targetAim = (targetPlayer != 0 && targetPlayer != 16) ? *(uintptr_t*)(targetPlayer - 16 + 1696) + 32 : 0;

		if (targetAim != 0 && targetAim == g_State.mouseAimData)
		{
			*(float*)(thisp + 64) += g_State.mouseAimPitch;
			*(float*)(thisp + 68) += g_State.mouseAimYaw;
			g_State.mouseAimPitch = 0.0f;
			g_State.mouseAimYaw = 0.0f;
		}
		else
		{
			frameTime = TARGET_FRAME_TIME;
		}
	}

	return OrbitCamera_Update.unsafe_fastcall<int>(thisp, frameTime);
}

static int __fastcall PlayerZGJumpSM_ProcessAimingControls_Hook(int thisp, float frameTime)
{
	if (g_State.isControllerActive)
	{
		return PlayerZGJumpSM_ProcessAimingControls.unsafe_fastcall<int>(thisp, frameTime);
	}

	float pitch, yaw;
	GetMouseRotation(1250.0f, pitch, yaw);

	// Convert to angular velocity (radians per second)
	const float SENSITIVITY = 20.0f;
	float inputX = yaw * SENSITIVITY;
	float inputY = pitch * SENSITIVITY;
	float aimFrameTime = TARGET_FRAME_TIME;

	if (*(BYTE*)(thisp + 404) == 0)
	{
		float deadZone = MemoryHelper::ReadMemory<float>(g_Addresses.ResponseCurvePtr + 0x8);
		float overflow = std::max({ std::abs(inputX), std::abs(inputY), 1.0f });

		inputX = std::copysign(deadZone + (1.0f - deadZone) * std::abs(inputX) / overflow, inputX);
		inputY = std::copysign(deadZone + (1.0f - deadZone) * std::abs(inputY) / overflow, inputY);
		aimFrameTime *= overflow;
	}

	// Update velocities
	*(float*)(thisp + 124) = inputX;
	*(float*)(thisp + 128) = inputY;

	return PlayerZGJumpSM_ProcessAimingControls.unsafe_fastcall<int>(thisp, aimFrameTime);
}

static void OnZeroGravityRotation(safetyhook::Context& ctx)
{
	if (!g_State.isControllerActive) return;

	// Gyro aiming, like on the ground
	uintptr_t player = *(uintptr_t*)(ctx.ebx + 80);
	if (!player || (*(DWORD*)(player + 664) & 0x80) == 0) return;

	*(float*)(ctx.esp + 0x18) += g_State.frameGyroYaw;
	*(float*)(ctx.esp + 0x24) += g_State.frameGyroPitch;
}

// =====================
// Aim updates
// =====================

static void OnRE4ChaseCameraAim(safetyhook::Context& ctx)
{
	if (g_State.isControllerActive) return;

	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	GetMouseRotation(1250.0f, pitch, yaw);

	yaw *= *(float*)(ctx.esp + 0x38) / MemoryHelper::ReadMemory<float>(g_Addresses.PlayerSpeedSettingsPtr + 16);
	pitch *= *(float*)(ctx.esp + 0x60) / MemoryHelper::ReadMemory<float>(g_Addresses.PlayerSpeedSettingsPtr + 28);

	float currentPitch = *(float*)ctx.ecx;
	update->deltaPitch = std::clamp(currentPitch + pitch, -PITCH_LIMIT_NORMAL, PITCH_LIMIT_NORMAL) - currentPitch;
	update->deltaYaw = yaw;
}

static void OnGroundAim(safetyhook::Context& ctx)
{
	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	if (!GetAimRotation(update->deltaPitch, update->deltaYaw, pitch, yaw)) return;

	if (!g_State.isControllerActive)
	{
		pitch *= GetAimSpeedModifier(32);
		yaw *= GetAimSpeedModifier(36);
	}

	float currentPitch = *(float*)ctx.ecx;
	update->deltaPitch = std::clamp(currentPitch + pitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;
	update->deltaYaw = yaw;
}

static void OnGroundAimPitch(safetyhook::Context& ctx)
{
	if (g_State.isControllerActive) return;

	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	GetMouseRotation(1500.0f, pitch, yaw);
	pitch *= GetAimSpeedModifier(32);

	float currentPitch = *(float*)ctx.ecx;
	update->deltaPitch = std::clamp(currentPitch + pitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;
	update->deltaYaw = 0.0f;
}

static void OnDraggedAim(safetyhook::Context& ctx)
{
	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	if (!GetAimRotation(update->deltaPitch, update->deltaYaw, pitch, yaw)) return;

	float pitchMin = -*(float*)(ctx.edi + 108) * DEG2RAD;
	float pitchMax = *(float*)(ctx.edi + 112) * DEG2RAD;
	float yawMax = *(float*)(ctx.edi + 116) * DEG2RAD;
	float yawMin = -*(float*)(ctx.edi + 120) * DEG2RAD;

	float* current = (float*)(ctx.ecx + 160);
	float pitchDelta = std::clamp(current[0] + pitch, pitchMin, pitchMax) - current[0];
	float yawDelta = std::clamp(current[1] + yaw, yawMin, yawMax) - current[1];

	if (g_State.isControllerActive)
	{
		update->deltaPitch = pitchDelta;
		update->deltaYaw = yawDelta;
		return;
	}

	RecordMouseAim(ctx.ecx, pitchDelta, yawDelta);

	float roll = 0.0f;
	uintptr_t settings = *(uintptr_t*)ctx.edi;
	if (*(BYTE*)(settings + 137) != 0)
	{
		float swayRoll = *(float*)(ctx.esi + 228);
		pitchDelta += *(float*)(ctx.esi + 224);
		yawDelta += swayRoll * MemoryHelper::ReadMemory<float>(g_Addresses.HangingYawFactorPtr);
		roll = swayRoll;
	}

	update->deltaPitch = pitchDelta;
	update->deltaYaw = yawDelta;
	update->deltaRoll = roll;
}

static void OnStationaryShootingAim(safetyhook::Context& ctx)
{
	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	if (!GetAimRotation(0.0f, 0.0f, pitch, yaw)) return;

	uintptr_t marker = *(uintptr_t*)(ctx.edi + 112);
	if (!marker) return;

	uintptr_t markerBase = marker - 16;
	uintptr_t boundsAddr;
	switch (*(int*)(markerBase + 588))
	{
		case 1:     boundsAddr = markerBase + 608; break;
		case 2:
		case 3:     boundsAddr = markerBase + 624; break;
		case 5:     boundsAddr = markerBase + 640; break;
		default:    boundsAddr = markerBase + 592; break;
	}

	float* bounds = (float*)boundsAddr;
	float* aimPitch = (float*)(ctx.edi + 96);
	float* aimYaw = (float*)(ctx.edi + 100);

	float newPitch = std::clamp(*aimPitch + pitch, bounds[1] * DEG2RAD, bounds[0] * DEG2RAD);
	float newYaw = std::clamp(*aimYaw + yaw, bounds[3] * DEG2RAD, bounds[2] * DEG2RAD);

	RecordMouseAim(ctx.ecx, newPitch - *aimPitch, newYaw - *aimYaw);
	*aimPitch = newPitch;
	*aimYaw = newYaw;

	update->deltaPitch = newPitch;
	update->deltaYaw = newYaw;
}

static void OnDecompressionAim(safetyhook::Context& ctx)
{
	uintptr_t target = *(uintptr_t*)(ctx.esi + 20);
	if (target == 0 || target == 16) return;

	uintptr_t settings = *(uintptr_t*)(target - 16 + 12);
	if (!settings) return;

	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	if (!GetAimRotation(update->deltaPitch, update->deltaYaw, pitch, yaw)) return;

	float pitchMin = -*(float*)(settings + 92);
	float pitchMax = *(float*)(settings + 96);
	float yawMaxLow = *(float*)(settings + 100);
	float yawMin = -*(float*)(settings + 104);
	float pitchThreshold = *(float*)(settings + 108);
	float yawMaxHigh = *(float*)(settings + 112);

	float* current = (float*)ctx.ecx;
	float newPitch = std::clamp(current[0] + pitch, pitchMin, pitchMax);

	// Past the threshold pitch, the yaw limit moves from its low to its high value
	float yawMax = yawMaxLow;
	float yawRange = yawMaxHigh - yawMaxLow;
	if (newPitch >= pitchThreshold && yawRange != 0.0f)
	{
		float slope = (pitchMax - pitchThreshold) / yawRange;
		if (slope != 0.0f)
		{
			float intercept = pitchThreshold - slope * yawMaxLow;
			yawMax = (newPitch - intercept) / slope;
		}
	}

	float newYaw = std::clamp(current[1] + yaw, yawMin, yawMax);

	RecordMouseAim(ctx.ecx, newPitch - current[0], newYaw - current[1]);
	update->deltaPitch = newPitch - current[0];
	update->deltaYaw = newYaw - current[1];
}

static void OnHangingAim(safetyhook::Context& ctx)
{
	AimUpdate* update = reinterpret_cast<AimUpdate*>(ctx.esp);
	float pitch, yaw;
	if (!GetAimRotation(0.0f, 0.0f, pitch, yaw)) return;

	float* aimPitch = (float*)(ctx.edi + 120);
	float* aimYaw = (float*)(ctx.edi + 124);
	float swayPitch = *(float*)(ctx.edi + 128);
	float swayRoll = *(float*)(ctx.edi + 132);

	float newPitch = std::clamp(*aimPitch + pitch, MemoryHelper::ReadMemory<float>(g_Addresses.HangingMinPitchPtr), MemoryHelper::ReadMemory<float>(g_Addresses.HangingMaxPitchPtr));
	float newYaw = std::clamp(*aimYaw + yaw, MemoryHelper::ReadMemory<float>(g_Addresses.HangingMinYawPtr), MemoryHelper::ReadMemory<float>(g_Addresses.HangingMaxYawPtr));

	RecordMouseAim(ctx.ecx, newPitch - *aimPitch, newYaw - *aimYaw);
	*aimPitch = newPitch;
	*aimYaw = newYaw;

	update->deltaPitch = newPitch + swayPitch;
	update->deltaYaw = swayRoll * MemoryHelper::ReadMemory<float>(g_Addresses.HangingYawFactorPtr) + newYaw;
	update->deltaRoll = swayRoll;
}

static UINT WINAPI GetRawInputData_Hook(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT result = hkGetRawInputData.unsafe_stdcall<UINT>(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	if (result != (UINT)-1 && uiCommand == RID_INPUT && pData != NULL)
	{
		RAWINPUT* raw = (RAWINPUT*)pData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			if (AchievementOverlay::IsVisible())
			{
				// Overlay open: hide movement and clicks from the game
				raw->data.mouse.lLastX = 0;
				raw->data.mouse.lLastY = 0;
				raw->data.mouse.usButtonFlags = 0;
			}
			else
			{
				g_State.rawMouseDeltaX += raw->data.mouse.lLastX;
				g_State.rawMouseDeltaY += raw->data.mouse.lLastY;
			}
		}
	}

	return result;
}

static void ApplyRawMouseInput()
{
	if (!RawMouseInput) return;

	DWORD addr_PlayerZGJumpSM_ProcessAimingControls = GetAddress(Addr::PlayerZGJumpSM_ProcessAimingControls);
	DWORD addr_PlayerFPSAimSM_ProcessGroundAiming = GetAddress(Addr::PlayerFPSAimSM_ProcessGroundAiming);

	ApplyControlConfiguration = HookHelper::CreateHook((void*)GetAddress(Addr::ApplyControlConfiguration), &ApplyControlConfiguration_Hook);
	UpdateMenuCursor = HookHelper::CreateHook((void*)GetAddress(Addr::UpdateMenuCursor), &UpdateMenuCursor_Hook);
	RE4ChaseCamera_Update = HookHelper::CreateHook((void*)GetAddress(Addr::RE4ChaseCamera_Update), &RE4ChaseCamera_Update_Hook);
	OrbitCamera_Update = HookHelper::CreateHook((void*)GetAddress(Addr::OrbitCamera_Update), &OrbitCamera_Update_Hook);
	PlayerZGJumpSM_ProcessAimingControls = HookHelper::CreateHook((void*)addr_PlayerZGJumpSM_ProcessAimingControls, &PlayerZGJumpSM_ProcessAimingControls_Hook);

	ZeroGravityRotation = safetyhook::create_mid(reinterpret_cast<void*>(addr_PlayerZGJumpSM_ProcessAimingControls + 0x305), OnZeroGravityRotation);

	RE4ChaseCameraAim = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::RE4ChaseCamera_UpdateState)), OnRE4ChaseCameraAim);
	GroundAim = safetyhook::create_mid(reinterpret_cast<void*>(addr_PlayerFPSAimSM_ProcessGroundAiming + 0xEE), OnGroundAim);
	GroundAimPitch = safetyhook::create_mid(reinterpret_cast<void*>(addr_PlayerFPSAimSM_ProcessGroundAiming + 0x18F), OnGroundAimPitch);
	DraggedAim = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::PlayerDraggedSM_AdjustAim)), OnDraggedAim);
	StationaryShootingAim = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::PlayerStationaryShootingSM_AdjustAim)), OnStationaryShootingAim);
	DecompressionAim = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::PlayerDecompressionReactComponent_AdjustCameraAndAim)), OnDecompressionAim);
	HangingAim = safetyhook::create_mid(reinterpret_cast<void*>(GetAddress(Addr::PlayerHangingSM_UpdateAim)), OnHangingAim);

	hkGetRawInputData = HookHelper::CreateHookAPI(L"user32.dll", "GetRawInputData", &GetRawInputData_Hook);

	g_Addresses.InputDeviceManagerPtr = GetAddress(Addr::InputDeviceManagerPtr);
	g_Addresses.HangingMinYawPtr = GetAddress(Addr::HangingMinYawPtr);
	g_Addresses.HangingMaxYawPtr = GetAddress(Addr::HangingMaxYawPtr);
	g_Addresses.HangingMinPitchPtr = GetAddress(Addr::HangingMinPitchPtr);
	g_Addresses.HangingMaxPitchPtr = GetAddress(Addr::HangingMaxPitchPtr);
	g_Addresses.HangingYawFactorPtr = GetAddress(Addr::HangingYawFactorPtr);
	g_Addresses.ResponseCurvePtr = GetAddress(Addr::ResponseCurvePtr);
	g_Addresses.PlayerSpeedSettingsPtr = GetAddress(Addr::PlayerSpeedSettingsPtr);

	SensitivityInterp = reinterpret_cast<decltype(SensitivityInterp)>(GetAddress(Addr::SensitivityInterp));
	PlayerSpeedSettings_GetGunModifier = reinterpret_cast<decltype(PlayerSpeedSettings_GetGunModifier)>(GetAddress(Addr::PlayerSpeedSettings_GetGunModifier));
}
