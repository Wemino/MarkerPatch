#pragma once

#include "../../Globals.cpp"
static inline unsigned __int64 PackAngles(float vertical, float horizontal)
{
	float angles[2] = { vertical, horizontal };
	return std::bit_cast<std::uint64_t>(angles);
}
static inline void ScaleRawInput(float rawX, float rawY, float divisor, float& outX, float& outY)
{
	outX = (rawX * g_State.mouseSens) / -divisor;
	outY = (rawY * g_State.mouseSens) / divisor;
}

// =====================
// RawMouseInput
// =====================

safetyhook::InlineHook ApplyControlConfiguration;
safetyhook::InlineHook UpdateMenuCursor;
safetyhook::InlineHook UpdateCameraTracking;
safetyhook::InlineHook UpdateZeroGravityCamera;
safetyhook::InlineHook UpdateCameraPosition;
safetyhook::InlineHook hkGetRawInputData;
safetyhook::InlineHook UpdateAimWithMomentum;
safetyhook::InlineHook UpdateBoundedAim;
safetyhook::InlineHook UpdateConeAim;
safetyhook::InlineHook UpdateOscillatingAim;
safetyhook::InlineHook UpdateWeaponPoseBlend;
static int(__thiscall* OriginalApplyCameraRotation)(int*, unsigned __int64, unsigned int, int) = nullptr;

static int __stdcall ApplyControlConfiguration_Hook(int a1)
{
	// Get the current mouse sensitivity
	g_State.isXInverted = *(BYTE*)(a1);
	g_State.isYInverted = *(BYTE*)(a1 + 1);
	g_State.mouseSens = *(float*)(a1 + 12);
	if (g_State.mouseSens == 0.0f) g_State.mouseSens = 0.005f; // we still want to use the mouse
	ControllerHelper::SetGyroInvertX(g_State.isXInverted);
	ControllerHelper::SetGyroInvertY(g_State.isYInverted);
	return ApplyControlConfiguration.stdcall<int>(a1);
}

static void __fastcall UpdateMenuCursor_Hook(int thisp, float a2)
{
	// Get input manager instance
	int inputManager = *(int*)g_Addresses.InputManagerPtr;

	// Check if controller is being used (18 = mouse)
	g_State.isControllerActive = (*(int*)(inputManager + 1400) != 18);

	UpdateMenuCursor.unsafe_fastcall<void>(thisp, a2);
}

static int __fastcall UpdateCameraTracking_Hook(int thisp, float a2)
{
	if (!g_State.isControllerActive)
	{
		a2 = TARGET_FRAME_TIME;
	}
	return UpdateCameraTracking.unsafe_fastcall<int>(thisp, a2);
}

static int __fastcall UpdateZeroGravityCamera_Hook(int thisp, float frametime)
{
	if (g_State.isControllerActive)
	{
		return UpdateZeroGravityCamera.unsafe_fastcall<int>(thisp, frametime);
	}

	float deltaX, deltaY;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 625.0f, deltaX, deltaY);

	// Apply invert controls
	if (g_State.isXInverted)
		deltaX = -deltaX;
	if (g_State.isYInverted)
		deltaY = -deltaY;

	// Convert to angular velocity (radians per second)
	const float SENSITIVITY = 20.0f;

	// Update velocities
	*(float*)(thisp + 124) = deltaX * SENSITIVITY;
	*(float*)(thisp + 128) = deltaY * SENSITIVITY;

	return UpdateZeroGravityCamera.unsafe_fastcall<int>(thisp, TARGET_FRAME_TIME);
}

static int __fastcall UpdateCameraPosition_Hook(int thisp, float a2)
{
	if (!g_State.isControllerActive)
	{
		// Framerate independant sensitivity
		a2 = 1.0f;
	}

	return UpdateCameraPosition.unsafe_fastcall<int>(thisp, a2);
}

static int __fastcall UpdateAimWithMomentum_Hook(int* thisp, int)
{
	uintptr_t self = reinterpret_cast<uintptr_t>(thisp);
	int v2 = *reinterpret_cast<int*>(self + 116);
	uintptr_t v3 = (v2 != 0 && v2 != 16) ? *reinterpret_cast<uintptr_t*>(self + 124) : 0;

	g_State.momentumAimOuter = thisp;
	g_State.momentumAimData = v3;
	int result = UpdateAimWithMomentum.unsafe_thiscall<int>(thisp);
	g_State.momentumAimOuter = nullptr;
	g_State.momentumAimData = 0;
	return result;
}

static int __fastcall UpdateBoundedAim_Hook(int* thisp, int)
{
	g_State.boundedAimOuter = thisp;
	int result = UpdateBoundedAim.unsafe_thiscall<int>(thisp);
	g_State.boundedAimOuter = nullptr;
	return result;
}

static int __fastcall UpdateConeAim_Hook(int* thisp, int)
{
	uintptr_t self = reinterpret_cast<uintptr_t>(thisp);
	uintptr_t wrapper = *reinterpret_cast<uintptr_t*>(self + 20);
	uintptr_t data = 0;
	if (wrapper)
	{
		uintptr_t base = wrapper - 16;
		if (base)
		{
			data = *reinterpret_cast<uintptr_t*>(base + 12);
		}
	}

	g_State.coneAimData = data;
	int result = UpdateConeAim.unsafe_fastcall<int>(thisp);
	g_State.coneAimData = 0;
	return result;
}

static int __fastcall UpdateOscillatingAim_Hook(int* thisp, int)
{
	g_State.oscillatingAimOuter = thisp;
	int result = UpdateOscillatingAim.unsafe_fastcall<int>(thisp);
	g_State.oscillatingAimOuter = nullptr;
	return result;
}

static int __fastcall AimingApplyRotation_Primary(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
	{
		if (ControllerHelper::IsGyroEnabled())
		{
			float gyroYaw, gyroPitch;
			ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

			if (gyroYaw != 0.0f || gyroPitch != 0.0f)
			{
				float angles[2];
				std::memcpy(angles, &a2, sizeof(angles));
				angles[0] += gyroPitch;
				angles[1] += gyroYaw;

				float currentPitch = *(float*)thisp;
				float newPitch = currentPitch + angles[0];
				angles[0] = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

				return OriginalApplyCameraRotation(thisp, PackAngles(angles[0], angles[1]), a3, a4);
			}
		}

		return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

	if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
}

static int __fastcall AimingApplyRotation_Secondary(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(0.0f, static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, PITCH_LIMIT_AIM_DOWN, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, 0.0f), a3, a4);
}

static int __fastcall PlayerApplyRotation(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	if (g_State.isControllerActive)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 625.0f, horizontalDelta, verticalDelta);

	if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
	if (g_State.isYInverted) verticalDelta = -verticalDelta;

	float currentPitch = *(float*)thisp;
	float newPitch = currentPitch + verticalDelta;
	verticalDelta = std::clamp(newPitch, -PITCH_LIMIT_NORMAL, PITCH_LIMIT_NORMAL) - currentPitch;

	return OriginalApplyCameraRotation(thisp, PackAngles(verticalDelta, horizontalDelta), a3, a4);
}

static int __fastcall ApplyAimWithMomentum(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.momentumAimOuter;
	uintptr_t v3 = g_State.momentumAimData;

	if (!outer || !v3)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;
	float rollDelta = 0.0f;
	bool momentumAlreadyApplied = false;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float gyroYaw, gyroPitch;
		ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

		if (gyroYaw == 0.0f && gyroPitch == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float angles[2];
		std::memcpy(angles, &a2, sizeof(angles));
		verticalDelta = angles[0] + gyroPitch;
		horizontalDelta = angles[1] + gyroYaw;

		std::memcpy(&rollDelta, &a3, sizeof(rollDelta));
		momentumAlreadyApplied = true;
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	float pitchMinAbs = *reinterpret_cast<float*>(v3 + 108) * DEG2RAD;
	float pitchMax = *reinterpret_cast<float*>(v3 + 112) * DEG2RAD;
	float yawMax = *reinterpret_cast<float*>(v3 + 116) * DEG2RAD;
	float yawMinAbs = *reinterpret_cast<float*>(v3 + 120) * DEG2RAD;

	float* snapshot = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisp) + 160);
	float currentPitch = snapshot[0];
	float currentYaw = snapshot[1];

	float newPitch = std::clamp(currentPitch + verticalDelta, -pitchMinAbs, pitchMax);
	float newYaw = std::clamp(currentYaw + horizontalDelta, -yawMinAbs, yawMax);

	float pitchDelta = newPitch - currentPitch;
	float yawDelta = newYaw - currentYaw;

	if (!momentumAlreadyApplied)
	{
		uintptr_t v3Class = *reinterpret_cast<uintptr_t*>(v3);
		if (v3Class && *reinterpret_cast<unsigned char*>(v3Class + 137) != 0)
		{
			uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);
			float v224 = *reinterpret_cast<float*>(outerAddr + 224);
			float v228 = *reinterpret_cast<float*>(outerAddr + 228);
			float rollYawCoef = MemoryHelper::ReadMemory<float>(g_Addresses.CameraRollToYawCoef);

			pitchDelta += v224;
			yawDelta += v228 * rollYawCoef;
			rollDelta = v228;
		}
	}

	unsigned int rollBits;
	std::memcpy(&rollBits, &rollDelta, sizeof(rollBits));
	return OriginalApplyCameraRotation(thisp, PackAngles(pitchDelta, yawDelta), rollBits, a4);
}

static int __fastcall ApplyBoundedAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.boundedAimOuter;
	if (!outer)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		ControllerHelper::GetProcessedGyroDelta(horizontalDelta, verticalDelta);

		if (horizontalDelta == 0.0f && verticalDelta == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	const uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);

	uintptr_t wrapper = *reinterpret_cast<uintptr_t*>(outerAddr + 112);
	if (!wrapper)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	uintptr_t v4 = wrapper - 16;
	uintptr_t clampBase;
	switch (*reinterpret_cast<int*>(v4 + 588))
	{
		case 1:     clampBase = v4 + 608; break;
		case 2:
		case 3:     clampBase = v4 + 624; break;
		case 5:     clampBase = v4 + 640; break;
		default:    clampBase = v4 + 592; break;
	}

	float* bounds = reinterpret_cast<float*>(clampBase);
	float pitchMax = bounds[0] * DEG2RAD;
	float pitchMin = bounds[1] * DEG2RAD;
	float yawMax = bounds[2] * DEG2RAD;
	float yawMin = bounds[3] * DEG2RAD;

	float* storedPitch = reinterpret_cast<float*>(outerAddr + 96);
	float* storedYaw = reinterpret_cast<float*>(outerAddr + 100);

	float newPitch = std::clamp(*storedPitch + verticalDelta, pitchMin, pitchMax);
	float newYaw = std::clamp(*storedYaw + horizontalDelta, yawMin, yawMax);

	*storedPitch = newPitch;
	*storedYaw = newYaw;

	return OriginalApplyCameraRotation(thisp, PackAngles(newPitch, newYaw), a3, a4);
}

static int __fastcall ApplyConeAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	uintptr_t d = g_State.coneAimData;
	if (!d)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float gyroYaw, gyroPitch;
		ControllerHelper::GetProcessedGyroDelta(gyroYaw, gyroPitch);

		if (gyroYaw == 0.0f && gyroPitch == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		float angles[2];
		std::memcpy(angles, &a2, sizeof(angles));
		verticalDelta = angles[0] + gyroPitch;
		horizontalDelta = angles[1] + gyroYaw;
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	float pitchMinAbs = *reinterpret_cast<float*>(d + 92);
	float pitchMax = *reinterpret_cast<float*>(d + 96);
	float yawMaxLow = *reinterpret_cast<float*>(d + 100);
	float yawMinAbs = *reinterpret_cast<float*>(d + 104);
	float pitchThresh = *reinterpret_cast<float*>(d + 108);
	float yawMaxHigh = *reinterpret_cast<float*>(d + 112);

	float* cam = reinterpret_cast<float*>(thisp);
	float currentPitch = cam[0];
	float currentYaw = cam[1];

	float newPitch = std::clamp(currentPitch + verticalDelta, -pitchMinAbs, pitchMax);

	float yawMax;
	float yawRange = yawMaxHigh - yawMaxLow;
	if (newPitch >= pitchThresh && yawRange != 0.0f)
	{
		float slope = (pitchMax - pitchThresh) / yawRange;
		if (slope != 0.0f)
		{
			float intercept = pitchThresh - slope * yawMaxLow;
			yawMax = (newPitch - intercept) / slope;
		}
		else
		{
			yawMax = yawMaxLow;
		}
	}
	else
	{
		yawMax = yawMaxLow;
	}

	float newYaw = std::clamp(currentYaw + horizontalDelta, -yawMinAbs, yawMax);

	return OriginalApplyCameraRotation(thisp, PackAngles(newPitch - currentPitch, newYaw - currentYaw), a3, a4);
}

static int __fastcall ApplyOscillatingAim(int* thisp, int, unsigned __int64 a2, unsigned int a3, int a4)
{
	int* outer = g_State.oscillatingAimOuter;
	if (!outer)
		return OriginalApplyCameraRotation(thisp, a2, a3, a4);

	float horizontalDelta = 0.0f, verticalDelta = 0.0f;

	if (g_State.isControllerActive)
	{
		if (!ControllerHelper::IsGyroEnabled())
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);

		ControllerHelper::GetProcessedGyroDelta(horizontalDelta, verticalDelta);

		if (horizontalDelta == 0.0f && verticalDelta == 0.0f)
			return OriginalApplyCameraRotation(thisp, a2, a3, a4);
	}
	else
	{
		ScaleRawInput(static_cast<float>(g_State.frameRawX), static_cast<float>(g_State.frameRawY), 750.0f, horizontalDelta, verticalDelta);

		if (g_State.isXInverted) horizontalDelta = -horizontalDelta;
		if (g_State.isYInverted) verticalDelta = -verticalDelta;
	}

	const uintptr_t outerAddr = reinterpret_cast<uintptr_t>(outer);

	float yawMin = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownYawMin);
	float yawMax = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownYawMax);
	float pitchMin = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownPitchMin);
	float pitchMax = MemoryHelper::ReadMemory<float>(g_Addresses.UpsideDownPitchMax);
	float rollYawCoef = MemoryHelper::ReadMemory<float>(g_Addresses.CameraRollToYawCoef);

	float* storedPitch = reinterpret_cast<float*>(outerAddr + 120);
	float* storedYaw = reinterpret_cast<float*>(outerAddr + 124);
	float pitchBob = *reinterpret_cast<float*>(outerAddr + 128);
	float rollBob = *reinterpret_cast<float*>(outerAddr + 132);

	float newPitch = std::clamp(*storedPitch + verticalDelta, pitchMin, pitchMax);
	float newYaw = std::clamp(*storedYaw + horizontalDelta, yawMin, yawMax);

	*storedPitch = newPitch;
	*storedYaw = newYaw;
	float packedPitch = newPitch + pitchBob;
	float packedYaw = (rollBob * rollYawCoef) + newYaw;
	unsigned int rollBits;
	std::memcpy(&rollBits, &rollBob, sizeof(rollBits));
	return OriginalApplyCameraRotation(thisp, PackAngles(packedPitch, packedYaw), rollBits, a4);
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

	DWORD addr_ApplyControlConfiguration = ScanModuleSignature(g_State.GameModule, "56 8B 74 24 08 0F B6 06 50 E8", "ApplyControlConfiguration");
	DWORD addr_UpdateMenuCursor = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 64 53 56 8B F1 F7 46 20 00 00 01 00", "UpdateMenuCursor");
	DWORD addr_UpdateCameraTracking = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 F3 0F 10 45 08 F3 0F 59 05 ?? ?? ?? ?? 81 EC A4 01 00 00", "UpdateCameraTracking");
	DWORD addr_UpdateZeroGravityCamera = ScanModuleSignature(g_State.GameModule, "83 EC 14 53 8B D9 80 BB 94 01 00 00 00", "UpdateZeroGravityCamera_Hook");
	DWORD addr_UpdateCameraPosition = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 F3 0F 10 45 08 D9 45 08", "UpdateCameraPosition");
	DWORD addr_UpdateAimingCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 64 01 00 00 A1 ?? ?? ?? ?? D9 45 0C 53 8B D9", "UpdateAimingCamera");
	DWORD addr_UpdatePlayerCamera = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 34 01 00 00 53 8B D9 8B 43 74", "UpdatePlayerCamera");
	DWORD addr_ApplyCameraRotation = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 54 F3 0F 10 45 08 8B 45", "ApplyCameraRotation");
	DWORD addr_UpdateAimWithMomentum = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 74 53 56 8B F1 8B 46 74", "UpdateAimWithMomentum");
	DWORD addr_UpdateBoundedAim = ScanModuleSignature(g_State.GameModule, "C3 CC 83 EC 20 D9 05 ?? ?? ?? ?? 53 56 D9 5C 24 08", "UpdateBoundedAim");
	DWORD addr_UpdateConeAim = ScanModuleSignature(g_State.GameModule, "83 EC 30 56 8B F1 8B 46 14 85 C0 0F 84", "UpdateConeAim");
	DWORD addr_UpdateOscillatingAim = ScanModuleSignature(g_State.GameModule, "83 EC 08 F3 0F 10 05 ?? ?? ?? ?? 53 56 57 8B F9", "UpdateOscillatingAim");

	if (addr_ApplyControlConfiguration == 0 ||
		addr_UpdateMenuCursor == 0 ||
		addr_UpdateCameraTracking == 0 ||
		addr_UpdateZeroGravityCamera == 0 ||
		addr_UpdateCameraPosition == 0 ||
		addr_UpdateAimingCamera == 0 ||
		addr_UpdatePlayerCamera == 0 ||
		addr_ApplyCameraRotation == 0 ||
		addr_UpdateAimWithMomentum == 0 ||
		addr_UpdateBoundedAim == 0 ||
		addr_UpdateConeAim == 0 ||
		addr_UpdateOscillatingAim == 0) {
		return;
	}

	ApplyControlConfiguration = HookHelper::CreateHook((void*)addr_ApplyControlConfiguration, &ApplyControlConfiguration_Hook);
	UpdateMenuCursor = HookHelper::CreateHook((void*)addr_UpdateMenuCursor, &UpdateMenuCursor_Hook);
	UpdateCameraTracking = HookHelper::CreateHook((void*)addr_UpdateCameraTracking, &UpdateCameraTracking_Hook);
	UpdateZeroGravityCamera = HookHelper::CreateHook((void*)addr_UpdateZeroGravityCamera, &UpdateZeroGravityCamera_Hook);
	UpdateCameraPosition = HookHelper::CreateHook((void*)addr_UpdateCameraPosition, &UpdateCameraPosition_Hook);
	UpdateAimWithMomentum = HookHelper::CreateHook((void*)addr_UpdateAimWithMomentum, &UpdateAimWithMomentum_Hook);
	UpdateBoundedAim = HookHelper::CreateHook((void*)(addr_UpdateBoundedAim + 0x2), &UpdateBoundedAim_Hook);
	UpdateConeAim = HookHelper::CreateHook((void*)addr_UpdateConeAim, &UpdateConeAim_Hook);
	UpdateOscillatingAim = HookHelper::CreateHook((void*)addr_UpdateOscillatingAim, &UpdateOscillatingAim_Hook);

	OriginalApplyCameraRotation = reinterpret_cast<decltype(OriginalApplyCameraRotation)>(addr_ApplyCameraRotation);
	MemoryHelper::MakeCALL(addr_UpdateAimingCamera + 0xEE, (uintptr_t)&AimingApplyRotation_Primary);
	MemoryHelper::MakeCALL(addr_UpdateAimingCamera + 0x18F, (uintptr_t)&AimingApplyRotation_Secondary);
	MemoryHelper::MakeCALL(addr_UpdatePlayerCamera + 0x7AE, (uintptr_t)&PlayerApplyRotation);
	MemoryHelper::MakeCALL(addr_UpdateAimWithMomentum + 0x4E0, (uintptr_t)&ApplyAimWithMomentum);
	MemoryHelper::MakeCALL(addr_UpdateBoundedAim + 0x206, (uintptr_t)&ApplyBoundedAim);
	MemoryHelper::MakeCALL(addr_UpdateConeAim + 0x2DC, (uintptr_t)&ApplyConeAim);
	MemoryHelper::MakeCALL(addr_UpdateOscillatingAim + 0x244, (uintptr_t)&ApplyOscillatingAim);

	hkGetRawInputData = HookHelper::CreateHookAPI(L"user32.dll", "GetRawInputData", &GetRawInputData_Hook);

	g_Addresses.InputManagerPtr = MemoryHelper::ReadMemory<int>(addr_UpdateMenuCursor + 0x29);
	g_Addresses.UpsideDownYawMin = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x18C);
	g_Addresses.UpsideDownYawMax = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1A8);
	g_Addresses.UpsideDownPitchMin = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1BD);
	g_Addresses.UpsideDownPitchMax = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x1CA);
	g_Addresses.CameraRollToYawCoef = MemoryHelper::ReadMemory<int>(addr_UpdateOscillatingAim + 0x214);
}
