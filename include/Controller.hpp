#pragma once

#include <SDL3/SDL.h>
#include <Windows.h>
#include <Xinput.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <string>

namespace ControllerHelper
{
	// ==========================================================
	// Types and Enums
	// ==========================================================

	enum class GamepadStyle
	{
		Xbox,
		PlayStation,
		Nintendo,
		Unknown
	};

	struct GamepadCapabilities
	{
		GamepadStyle style = GamepadStyle::Unknown;
		bool hasTouchpad = false;
		bool hasGyro = false;
		bool hasAccel = false;
		const char* name = nullptr;
		Uint16 vendorId = 0;
		Uint16 productId = 0;
	};

	struct GyroState
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		bool isValid = false;
	};

	struct GyroConfig
	{
		bool isEnabled = true;
		float sensitivity = 1.0f;
		float smoothing = 0.016f;
		bool invertX = false;
		bool invertY = false;
	};

	struct GyroProcessingState
	{
		float smoothedYaw = 0.0f;
		float smoothedPitch = 0.0f;
		LARGE_INTEGER lastFrameTime = {};
		LARGE_INTEGER frequency = {};
		bool isInitialized = false;
	};

	struct GyroAutoOffset
	{
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		float offsetZ = 0.0f;

		static constexpr int BUFFER_SIZE = 256;
		float bufferX[BUFFER_SIZE] = {};
		float bufferY[BUFFER_SIZE] = {};
		float bufferZ[BUFFER_SIZE] = {};
		float bufferDt[BUFFER_SIZE] = {};
		int bufferIndex = 0;
		int sampleCount = 0;

		double sumDt = 0.0;
		float resyncAccumulator = 0.0f;
		float pendingDt = 0.0f;

		float stillnessTimer = 0.0f;
		bool hasInitialCalibration = false;

		float startGravX = 0.0f;
		float startGravY = 0.0f;
		float startGravZ = 0.0f;
		bool hasStartGrav = false;
		float maxGravAngleDuringWindow = 0.0f;

		float smAccelX = 0.0f;
		float smAccelY = 0.0f;
		float smAccelZ = 0.0f;
		bool hasSmoothedAccel = false;

		float stabilityCooldownTimer = 0.0f;
		bool isSurfaceMode = false;
	};

	struct TouchpadConfig
	{
		bool isEnabled = true;
		int currentWidth = 0;
		int currentHeight = 0;
	};

	struct TouchpadState
	{
		bool wasDown = false;
		float lastX = 0.0f;
		float lastY = 0.0f;
	};

	struct FrameTiming
	{
		LARGE_INTEGER lastFrameTime = {};
		LARGE_INTEGER frequency = {};
		float deltaTime = 0.0f;
	};

	// ==========================================================
	// Static State
	// ==========================================================

	inline SDL_Gamepad* s_pGamepad = nullptr;
	inline GamepadCapabilities s_capabilities;

	inline GyroState s_gyroState;
	inline GyroConfig s_gyroConfig;
	inline GyroProcessingState s_gyroProcessing;
	inline GyroAutoOffset s_gyroOffset;

	inline TouchpadConfig s_touchpadConfig;
	static constexpr int MAX_TOUCHPADS = 2;
	static constexpr int MAX_FINGERS = 2;

	inline TouchpadState s_touchpadFinger[MAX_TOUCHPADS][MAX_FINGERS];
	inline bool s_wasTouchpadPressed[MAX_TOUCHPADS] = {};

	inline FrameTiming s_frameTiming;

	inline bool s_gyroCalibrationPersistenceEnabled = true;
	inline bool s_gyroCalibrationSavedThisSession = false;
	inline std::string s_currentControllerSerial;

	// ==========================================================
	// Gyro Calibration Persistence
	// ==========================================================

	static std::string GetGyroCalibrationFolder()
	{
		return "GyroCalibration\\";
	}

	static std::string GetGyroCalibrationFilePath(const char* serial)
	{
		if (!serial || serial[0] == '\0')
			return "";

		std::string sanitized;
		for (const char* p = serial; *p; p++)
		{
			char c = *p;
			if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
				sanitized += c;
		}

		if (sanitized.empty())
			return "";

		return GetGyroCalibrationFolder() + sanitized + ".gyro";
	}

	static void SaveGyroCalibration()
	{
		if (s_currentControllerSerial.empty())
			return;

		std::string folderPath = GetGyroCalibrationFolder();
		CreateDirectoryA(folderPath.c_str(), nullptr);

		std::string filePath = GetGyroCalibrationFilePath(s_currentControllerSerial.c_str());
		if (filePath.empty())
			return;

		std::ofstream file(filePath, std::ios::binary);
		if (!file)
			return;

		file.write(reinterpret_cast<const char*>(&s_gyroOffset.offsetX), sizeof(float));
		file.write(reinterpret_cast<const char*>(&s_gyroOffset.offsetY), sizeof(float));
		file.write(reinterpret_cast<const char*>(&s_gyroOffset.offsetZ), sizeof(float));

		s_gyroCalibrationSavedThisSession = true;
	}

	static bool LoadGyroCalibration(const char* serial)
	{
		if (!serial || serial[0] == '\0')
			return false;

		std::string filePath = GetGyroCalibrationFilePath(serial);
		if (filePath.empty())
			return false;

		std::ifstream file(filePath, std::ios::binary);
		if (!file)
			return false;

		file.seekg(0, std::ios::end);
		if (file.tellg() != 0xC)
			return false;

		file.seekg(0, std::ios::beg);

		float offsetX = 0.0, offsetY = 0.0, offsetZ = 0.0;
		file.read(reinterpret_cast<char*>(&offsetX), sizeof(float));
		file.read(reinterpret_cast<char*>(&offsetY), sizeof(float));
		file.read(reinterpret_cast<char*>(&offsetZ), sizeof(float));

		if (!file)
			return false;

		s_gyroOffset.offsetX = offsetX;
		s_gyroOffset.offsetY = offsetY;
		s_gyroOffset.offsetZ = offsetZ;
		s_gyroOffset.hasInitialCalibration = true;

		return true;
	}

	// ==========================================================
	// Frame Timing
	// ==========================================================

	inline void UpdateFrameTiming()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		s_frameTiming.deltaTime = static_cast<float>(currentTime.QuadPart - s_frameTiming.lastFrameTime.QuadPart) / static_cast<float>(s_frameTiming.frequency.QuadPart);

		s_frameTiming.deltaTime = std::clamp(s_frameTiming.deltaTime, 0.0001f, 0.1f);
		s_frameTiming.lastFrameTime = currentTime;
	}

	// ==========================================================
	// Controller Database
	// ==========================================================

	inline bool LoadGamepadMappings()
	{
		const char* paths[] =
		{
			"gamecontrollerdb.txt",
			"../gamecontrollerdb.txt",
		};

		for (const char* path : paths)
		{
			int result = SDL_AddGamepadMappingsFromFile(path);
			if (result >= 0)
			{
				return true;
			}
		}

		return false;
	}

	// ==========================================================
	// Capability Detection
	// ==========================================================

	inline GamepadStyle DetectGamepadStyle(SDL_Gamepad* pGamepad)
	{
		if (!pGamepad)
			return GamepadStyle::Unknown;

		SDL_GamepadType type = SDL_GetGamepadType(pGamepad);

		switch (type)
		{
			case SDL_GAMEPAD_TYPE_PS3:
			case SDL_GAMEPAD_TYPE_PS4:
			case SDL_GAMEPAD_TYPE_PS5:
				return GamepadStyle::PlayStation;

			default:
				break;
		}

		if (SDL_GetGamepadButtonLabel(pGamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_B)
			return GamepadStyle::Nintendo;

		return GamepadStyle::Xbox;
	}

	inline void LoadGamepadCapabilities(SDL_Gamepad* pGamepad)
	{
		s_capabilities = GamepadCapabilities();

		if (!pGamepad)
			return;

		s_capabilities.style = DetectGamepadStyle(pGamepad);
		s_capabilities.hasTouchpad = SDL_GetNumGamepadTouchpads(pGamepad) > 0;
		s_capabilities.hasGyro = SDL_GamepadHasSensor(pGamepad, SDL_SENSOR_GYRO);
		s_capabilities.hasAccel = SDL_GamepadHasSensor(pGamepad, SDL_SENSOR_ACCEL);
		s_capabilities.name = SDL_GetGamepadName(pGamepad);
		s_capabilities.vendorId = SDL_GetGamepadVendor(pGamepad);
		s_capabilities.productId = SDL_GetGamepadProduct(pGamepad);

		if (s_capabilities.hasGyro)
		{
			SDL_SetGamepadSensorEnabled(pGamepad, SDL_SENSOR_GYRO, true);

			const char* serial = SDL_GetGamepadSerial(pGamepad);
			if (serial && serial[0] != '\0')
			{
				s_currentControllerSerial = serial;

				if (s_gyroCalibrationPersistenceEnabled)
				{
					LoadGyroCalibration(serial);
				}
			}
			else
			{
				s_currentControllerSerial.clear();
			}

			s_gyroCalibrationSavedThisSession = false;
		}

		if (s_capabilities.hasAccel)
		{
			SDL_SetGamepadSensorEnabled(pGamepad, SDL_SENSOR_ACCEL, true);
		}
	}

	inline void ResetControllerState()
	{
		s_capabilities = GamepadCapabilities();
		s_gyroState = GyroState();
		s_gyroProcessing = GyroProcessingState();
		s_gyroOffset = GyroAutoOffset();
		s_gyroCalibrationSavedThisSession = false;
		s_currentControllerSerial.clear();

		for (int tp = 0; tp < MAX_TOUCHPADS; tp++)
		{
			for (int f = 0; f < MAX_FINGERS; f++)
			{
				s_touchpadFinger[tp][f] = TouchpadState();
			}
			s_wasTouchpadPressed[tp] = false;
		}
	}

	// ==========================================================
	// Initialization / Shutdown
	// ==========================================================

	inline bool InitializeSDLGamepad()
	{
		if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
		{
			SDL_Quit();
			return false;
		}

		LoadGamepadMappings();

		const SDL_EventType unusedEvents[] =
		{
			SDL_EVENT_JOYSTICK_AXIS_MOTION, SDL_EVENT_JOYSTICK_BALL_MOTION, SDL_EVENT_JOYSTICK_HAT_MOTION,
			SDL_EVENT_JOYSTICK_BUTTON_DOWN, SDL_EVENT_JOYSTICK_BUTTON_UP,
			SDL_EVENT_GAMEPAD_AXIS_MOTION, SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP,
			SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN, SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION, SDL_EVENT_GAMEPAD_TOUCHPAD_UP,
			SDL_EVENT_GAMEPAD_SENSOR_UPDATE,
		};
		for (SDL_EventType eventType : unusedEvents)
		{
			SDL_SetEventEnabled(eventType, false);
		}

		QueryPerformanceFrequency(&s_frameTiming.frequency);
		QueryPerformanceCounter(&s_frameTiming.lastFrameTime);

		return true;
	}

	inline void ShutdownSDLGamepad()
	{
		if (s_pGamepad)
		{
			SDL_CloseGamepad(s_pGamepad);
			s_pGamepad = nullptr;
		}

		ResetControllerState();
		s_frameTiming = FrameTiming();

		SDL_Quit();
	}

	// ==========================================================
	// Accessors - Controller
	// ==========================================================

	inline SDL_Gamepad* GetGamepad()
	{
		return s_pGamepad;
	}

	inline const GamepadCapabilities& GetCapabilities()
	{
		return s_capabilities;
	}

	inline GamepadStyle GetGamepadStyle()
	{
		return s_capabilities.style;
	}

	inline bool IsConnected()
	{
		return s_pGamepad && SDL_GamepadConnected(s_pGamepad);
	}

	// ==========================================================
	// Accessors - Gyro
	// ==========================================================

	inline bool HasGyro()
	{
		return s_capabilities.hasGyro;
	}

	inline bool IsGyroEnabled()
	{
		return s_gyroConfig.isEnabled && s_capabilities.hasGyro;
	}

	inline const GyroState& GetGyroState()
	{
		return s_gyroState;
	}

	// ==========================================================
	// Accessors - Touchpad
	// ==========================================================

	inline bool HasTouchpad()
	{
		return s_capabilities.hasTouchpad;
	}

	// ==========================================================
	// Configuration - Gyro
	// ==========================================================

	inline void SetGyroEnabled(bool enabled)
	{
		s_gyroConfig.isEnabled = enabled;
	}

	inline void SetGyroSensitivity(float sensitivity)
	{
		s_gyroConfig.sensitivity = std::max(sensitivity, 0.0f);
	}

	inline void SetGyroSmoothing(float smoothing)
	{
		s_gyroConfig.smoothing = std::clamp(smoothing, 0.001f, 0.1f);
	}

	inline void SetGyroInvertX(bool invert)
	{
		s_gyroConfig.invertX = invert;
	}

	inline void SetGyroInvertY(bool invert)
	{
		s_gyroConfig.invertY = invert;
	}

	inline void SetGyroCalibrationPersistence(bool enabled)
	{
		s_gyroCalibrationPersistenceEnabled = enabled;
	}

	inline void ResetGyroState()
	{
		s_gyroProcessing.smoothedYaw = 0.0f;
		s_gyroProcessing.smoothedPitch = 0.0f;
		s_gyroProcessing.isInitialized = false;
		s_gyroOffset = GyroAutoOffset();
	}

	// ==========================================================
	// Configuration - Touchpad
	// ==========================================================

	inline void SetTouchpadEnabled(bool enabled)
	{
		s_touchpadConfig.isEnabled = enabled;
	}

	inline void SetTouchpadDimensions(int width, int height)
	{
		s_touchpadConfig.currentWidth = width;
		s_touchpadConfig.currentHeight = height;
	}

	// ==========================================================
	// Gyro Auto-Calibration
	// ==========================================================

	inline void ResetStillnessTracking()
	{
		s_gyroOffset.stillnessTimer = 0.0f;
		s_gyroOffset.hasStartGrav = false;
		s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
	}

	inline void UpdateGyroOffset(float gyroX, float gyroY, float gyroZ, float accelX, float accelY, float accelZ, float deltaTime)
	{
		if (!std::isfinite(deltaTime) || deltaTime <= 0.0f)
			return;

		float dt = fminf(deltaTime, 0.1f);

		if (!std::isfinite(gyroX) || !std::isfinite(gyroY) || !std::isfinite(gyroZ))
			return;
		if (!std::isfinite(accelX) || !std::isfinite(accelY) || !std::isfinite(accelZ))
			return;

		// Freeze guard: ignore exact duplicates since real sensors always have noise
		if (s_gyroOffset.sampleCount > 0)
		{
			int prevIdx = (s_gyroOffset.bufferIndex - 1 + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
			if (gyroX == s_gyroOffset.bufferX[prevIdx] && gyroY == s_gyroOffset.bufferY[prevIdx] && gyroZ == s_gyroOffset.bufferZ[prevIdx])
			{
				s_gyroOffset.pendingDt = fminf(s_gyroOffset.pendingDt + dt, 0.1f);
				return;
			}
		}

		// The second fminf is intentional. First cap limits incoming deltaTime, second cap
		// limits the sum after adding pendingDt. This ensures dt never exceeds 0.1s even when
		// accumulated time from skipped duplicate frames is added.
		dt = fminf(dt + s_gyroOffset.pendingDt, 0.1f);
		s_gyroOffset.pendingDt = 0.0f;

		// ==========================================================
		// Update Ring Buffer
		// ==========================================================

		int idx = s_gyroOffset.bufferIndex;

		if (s_gyroOffset.sampleCount >= GyroAutoOffset::BUFFER_SIZE)
		{
			s_gyroOffset.sumDt -= s_gyroOffset.bufferDt[idx];
		}

		s_gyroOffset.bufferX[idx] = gyroX;
		s_gyroOffset.bufferY[idx] = gyroY;
		s_gyroOffset.bufferZ[idx] = gyroZ;
		s_gyroOffset.bufferDt[idx] = dt;

		s_gyroOffset.sumDt += dt;

		s_gyroOffset.bufferIndex = (idx + 1) % GyroAutoOffset::BUFFER_SIZE;

		if (s_gyroOffset.sampleCount < GyroAutoOffset::BUFFER_SIZE)
			s_gyroOffset.sampleCount++;

		s_gyroOffset.resyncAccumulator += dt;
		if (s_gyroOffset.resyncAccumulator >= 2.0f)
		{
			s_gyroOffset.resyncAccumulator = 0.0f;
			s_gyroOffset.sumDt = 0.0;
			for (int i = 0; i < s_gyroOffset.sampleCount; i++)
			{
				int bufIdx = (s_gyroOffset.bufferIndex - s_gyroOffset.sampleCount + i + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
				s_gyroOffset.sumDt += s_gyroOffset.bufferDt[bufIdx];
			}
		}

		bool isFull = (s_gyroOffset.sampleCount >= GyroAutoOffset::BUFFER_SIZE);
		if ((s_gyroOffset.sumDt < 0.5f && !isFull) || s_gyroOffset.sampleCount < 2)
			return;

		// ==========================================================
		// Smooth Accelerometer
		// ==========================================================

		const float accelTau = 0.1f;
		float accelAlpha = 1.0f - expf(-dt / accelTau);

		if (!s_gyroOffset.hasSmoothedAccel)
		{
			s_gyroOffset.smAccelX = accelX;
			s_gyroOffset.smAccelY = accelY;
			s_gyroOffset.smAccelZ = accelZ;
			s_gyroOffset.hasSmoothedAccel = true;
		}
		else
		{
			s_gyroOffset.smAccelX += (accelX - s_gyroOffset.smAccelX) * accelAlpha;
			s_gyroOffset.smAccelY += (accelY - s_gyroOffset.smAccelY) * accelAlpha;
			s_gyroOffset.smAccelZ += (accelZ - s_gyroOffset.smAccelZ) * accelAlpha;
		}

		// ==========================================================
		// Thresholds
		// ==========================================================

		// Thresholds in radians (gravity angles) and rad/s (gyro rates)
		const float THRESH_GRAV_INIT = 0.0026f;       // 0.15 deg
		const float THRESH_GRAV_REFINE = 0.00087f;    // 0.05 deg
		const float THRESH_GRAV_SURFACE = 0.0035f;    // 0.20 deg
		const float THRESH_TREND = 0.0007f;           // 0.04 deg/s
		const float THRESH_INIT_SIGNAL = 0.35f;       // 20 deg/s
		const float THRESH_REFINE_HANDHELD = 0.0026f; // 0.15 deg/s
		const float THRESH_REFINE_SURFACE = 0.035f;   // 2.0 deg/s
		const float THRESH_RANGE = 0.17f;             // 10 deg/s peak to peak

		// ==========================================================
		// Gate 1: Gravity Magnitude
		// ==========================================================

		float accLenSq = s_gyroOffset.smAccelX * s_gyroOffset.smAccelX + s_gyroOffset.smAccelY * s_gyroOffset.smAccelY + s_gyroOffset.smAccelZ * s_gyroOffset.smAccelZ;

		if (accLenSq < 64.0f || accLenSq > 132.0f)
		{
			ResetStillnessTracking();
			return;
		}

		// ==========================================================
		// Cooldown
		// ==========================================================

		if (s_gyroOffset.stabilityCooldownTimer > 0.0f)
		{
			s_gyroOffset.stabilityCooldownTimer -= dt;
			s_gyroOffset.stillnessTimer = 0.0f;
			s_gyroOffset.hasStartGrav = false;
			return;
		}

		// ==========================================================
		// Anchor Initialization
		// ==========================================================

		float accLen = sqrtf(accLenSq);
		float normGravX = s_gyroOffset.smAccelX / accLen;
		float normGravY = s_gyroOffset.smAccelY / accLen;
		float normGravZ = s_gyroOffset.smAccelZ / accLen;

		if (!s_gyroOffset.hasStartGrav)
		{
			s_gyroOffset.startGravX = normGravX;
			s_gyroOffset.startGravY = normGravY;
			s_gyroOffset.startGravZ = normGravZ;
			s_gyroOffset.hasStartGrav = true;
			s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
		}

		// ==========================================================
		// Gate 2: Gravity Stability
		// ==========================================================

		float dot = s_gyroOffset.startGravX * normGravX + s_gyroOffset.startGravY * normGravY + s_gyroOffset.startGravZ * normGravZ;

		float gravAngle = acosf(fmaxf(-1.0f, fminf(1.0f, dot)));

		if (gravAngle > s_gyroOffset.maxGravAngleDuringWindow)
			s_gyroOffset.maxGravAngleDuringWindow = gravAngle;

		float gravLimit = s_gyroOffset.hasInitialCalibration ? (s_gyroOffset.isSurfaceMode ? THRESH_GRAV_SURFACE : THRESH_GRAV_REFINE) : THRESH_GRAV_INIT;

		if (s_gyroOffset.maxGravAngleDuringWindow > gravLimit)
		{
			// Large movement (about 3 degrees) means the controller was picked up or put down.
			// This is the primary mechanism for exiting surface mode. The hysteresis exit in
			// Gate 3 is technically unreachable because you must pass this gate first, but the
			// large movement check here handles all real world pickup scenarios.
			if (s_gyroOffset.maxGravAngleDuringWindow > 0.05f)
				s_gyroOffset.isSurfaceMode = false;

			ResetStillnessTracking();
			s_gyroOffset.stabilityCooldownTimer = 1.0f;
			return;
		}

		// ==========================================================
		// Calculate Statistics (Time-Weighted)
		// ==========================================================

		// Time weighted statistics. We split the buffer into two halves by time to detect slow
		// drift (trend). Assigning boundary samples entirely to one bucket rather than splitting
		// them proportionally introduces at most 0.8% error at typical poll rates, which is
		// negligible for trend detection purposes.
		double sum1WX = 0, sum1WY = 0, sum1WZ = 0, time1 = 0;
		double sum2WX = 0, sum2WY = 0, sum2WZ = 0, time2 = 0;
		double halfTime = s_gyroOffset.sumDt * 0.5;
		double accumulatedTime = 0;

		int newestIdx = (s_gyroOffset.bufferIndex - 1 + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
		float minX = s_gyroOffset.bufferX[newestIdx], maxX = minX;
		float minY = s_gyroOffset.bufferY[newestIdx], maxY = minY;
		float minZ = s_gyroOffset.bufferZ[newestIdx], maxZ = minZ;

		for (int i = 0; i < s_gyroOffset.sampleCount; i++)
		{
			int bufIdx = (s_gyroOffset.bufferIndex - s_gyroOffset.sampleCount + i + GyroAutoOffset::BUFFER_SIZE) % GyroAutoOffset::BUFFER_SIZE;
			float gx = s_gyroOffset.bufferX[bufIdx];
			float gy = s_gyroOffset.bufferY[bufIdx];
			float gz = s_gyroOffset.bufferZ[bufIdx];
			float sampleDt = s_gyroOffset.bufferDt[bufIdx];

			if (gx < minX) minX = gx;
			if (gx > maxX) maxX = gx;
			if (gy < minY) minY = gy;
			if (gy > maxY) maxY = gy;
			if (gz < minZ) minZ = gz;
			if (gz > maxZ) maxZ = gz;

			if (accumulatedTime < halfTime)
			{
				sum1WX += gx * sampleDt;
				sum1WY += gy * sampleDt;
				sum1WZ += gz * sampleDt;
				time1 += sampleDt;
			}
			else
			{
				sum2WX += gx * sampleDt;
				sum2WY += gy * sampleDt;
				sum2WZ += gz * sampleDt;
				time2 += sampleDt;
			}
			accumulatedTime += sampleDt;
		}

		double totalTime = time1 + time2;
		float meanX = (float)((sum1WX + sum2WX) / totalTime);
		float meanY = (float)((sum1WY + sum2WY) / totalTime);
		float meanZ = (float)((sum1WZ + sum2WZ) / totalTime);
		float meanMag = sqrtf(meanX * meanX + meanY * meanY + meanZ * meanZ);

		// ==========================================================
		// Gate 3: Signal Guard
		// ==========================================================

		float maxRange = fmaxf(maxX - minX, fmaxf(maxY - minY, maxZ - minZ));

		if (maxRange > THRESH_RANGE)
		{
			ResetStillnessTracking();
			return;
		}

		if (!s_gyroOffset.hasInitialCalibration)
		{
			if (meanMag > THRESH_INIT_SIGNAL)
			{
				ResetStillnessTracking();
				return;
			}

			// Yaw spin detection prevents calibrating while the controller is being rotated flat
			// on a table (where gravity doesn't change). The 0.087 rad/s threshold (5 deg/s)
			// allows factory bias up to about 3 deg/s while catching real table spins. The check
			// only runs above this threshold so small meanMag values don't cause ratio instability.
			if (meanMag > 0.087f)
			{
				float yaw = fabsf(meanX * normGravX + meanY * normGravY + meanZ * normGravZ);
				if (yaw > meanMag * 0.9f)
				{
					ResetStillnessTracking();
					return;
				}
			}
		}
		else
		{
			float dx = meanX - s_gyroOffset.offsetX;
			float dy = meanY - s_gyroOffset.offsetY;
			float dz = meanZ - s_gyroOffset.offsetZ;
			float deviation = sqrtf(dx * dx + dy * dy + dz * dz);

			// Surface mode uses hysteresis to prevent thrashing. Enter threshold is tighter than
			// exit threshold. The 1 second stability requirement prevents false positives from
			// brief moments of steady hands during gameplay.
			const float SURFACE_ENTER_THRESH = 0.0017f;

			if (!s_gyroOffset.isSurfaceMode)
			{
				if (s_gyroOffset.maxGravAngleDuringWindow < SURFACE_ENTER_THRESH && s_gyroOffset.stillnessTimer > 1.0f)
					s_gyroOffset.isSurfaceMode = true;
			}

			float deviationLimit = s_gyroOffset.isSurfaceMode ? THRESH_REFINE_SURFACE : THRESH_REFINE_HANDHELD;

			if (deviation > deviationLimit)
			{
				ResetStillnessTracking();
				s_gyroOffset.stabilityCooldownTimer = 0.5f;
				return;
			}

			if (s_gyroOffset.isSurfaceMode && meanMag > 0.087f)
			{
				float yaw = fabsf(meanX * normGravX + meanY * normGravY + meanZ * normGravZ);
				if (yaw > meanMag * 0.9f)
				{
					ResetStillnessTracking();
					return;
				}
			}
		}

		// ==========================================================
		// Gate 4: Trend Stability
		// ==========================================================

		if (time1 <= 0.0 || time2 <= 0.0)
			return;

		float m1x = (float)(sum1WX / time1);
		float m1y = (float)(sum1WY / time1);
		float m1z = (float)(sum1WZ / time1);

		float m2x = (float)(sum2WX / time2);
		float m2y = (float)(sum2WY / time2);
		float m2z = (float)(sum2WZ / time2);

		float trend = fmaxf(fabsf(m2x - m1x), fmaxf(fabsf(m2y - m1y), fabsf(m2z - m1z)));

		if (trend > THRESH_TREND)
		{
			ResetStillnessTracking();
			s_gyroOffset.stabilityCooldownTimer = 0.5f;
			return;
		}

		// ==========================================================
		// Calibration
		// ==========================================================

		s_gyroOffset.stillnessTimer += dt;
		float requiredTime = s_gyroOffset.hasInitialCalibration ? 2.5f : 2.0f;

		if (s_gyroOffset.stillnessTimer > requiredTime)
		{
			if (!s_gyroOffset.hasInitialCalibration)
			{
				s_gyroOffset.offsetX = meanX;
				s_gyroOffset.offsetY = meanY;
				s_gyroOffset.offsetZ = meanZ;
				s_gyroOffset.hasInitialCalibration = true;

				// Anchor reset only happens after initial calibration. During refinement the
				// anchor persists to prevent drift creep from slow continuous rotation.
				s_gyroOffset.hasStartGrav = false;
				s_gyroOffset.maxGravAngleDuringWindow = 0.0f;
			}
			else
			{
				float updateInterval = requiredTime * 0.5f;
				const float DRIFT_CORRECTION_SPEED = 0.0026f;
				float maxStep = DRIFT_CORRECTION_SPEED * updateInterval;

				float diffX = meanX - s_gyroOffset.offsetX;
				float diffY = meanY - s_gyroOffset.offsetY;
				float diffZ = meanZ - s_gyroOffset.offsetZ;

				s_gyroOffset.offsetX += fmaxf(-maxStep, fminf(maxStep, diffX));
				s_gyroOffset.offsetY += fmaxf(-maxStep, fminf(maxStep, diffY));
				s_gyroOffset.offsetZ += fmaxf(-maxStep, fminf(maxStep, diffZ));
			}

			if (s_gyroCalibrationPersistenceEnabled && !s_gyroCalibrationSavedThisSession && !s_currentControllerSerial.empty())
			{
				SaveGyroCalibration();
			}

			s_gyroOffset.stillnessTimer = requiredTime * 0.5f;
		}
	}

	// ==========================================================
	// Processing - Gyro (Internal)
	// ==========================================================

	inline void ProcessGyro()
	{
		s_gyroState.isValid = false;

		if (!s_pGamepad || !s_gyroConfig.isEnabled || !s_capabilities.hasGyro)
			return;

		float gyro[3] = { 0.0f, 0.0f, 0.0f };
		float accel[3] = { 0.0f, 0.0f, 0.0f };

		if (SDL_GetGamepadSensorData(s_pGamepad, SDL_SENSOR_GYRO, gyro, 3) && SDL_GetGamepadSensorData(s_pGamepad, SDL_SENSOR_ACCEL, accel, 3))
		{
			UpdateGyroOffset(gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2], s_frameTiming.deltaTime);

			s_gyroState.x = gyro[0] - s_gyroOffset.offsetX;
			s_gyroState.y = gyro[1] - s_gyroOffset.offsetY;
			s_gyroState.z = gyro[2] - s_gyroOffset.offsetZ;
			s_gyroState.isValid = true;
		}
	}

	// ==========================================================
	// Processing - Gyro (Public)
	// ==========================================================

	inline void GetProcessedGyroDelta(float& outYaw, float& outPitch)
	{
		outYaw = 0.0f;
		outPitch = 0.0f;

		if (!s_gyroConfig.isEnabled || !s_capabilities.hasGyro || !s_gyroState.isValid)
			return;

		// Initialize timing on first call
		if (!s_gyroProcessing.isInitialized)
		{
			QueryPerformanceFrequency(&s_gyroProcessing.frequency);
			QueryPerformanceCounter(&s_gyroProcessing.lastFrameTime);
			s_gyroProcessing.isInitialized = true;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		float deltaTime = static_cast<float>(currentTime.QuadPart - s_gyroProcessing.lastFrameTime.QuadPart) / static_cast<float>(s_gyroProcessing.frequency.QuadPart);
		s_gyroProcessing.lastFrameTime = currentTime;
		deltaTime = std::clamp(deltaTime, 0.0001f, 0.1f);

		float rawYaw = s_gyroState.y;
		float rawPitch = -s_gyroState.x;

		if (s_gyroConfig.invertX) rawYaw = -rawYaw;
		if (s_gyroConfig.invertY) rawPitch = -rawPitch;

		float smoothFactor = expf(-deltaTime / fmaxf(s_gyroConfig.smoothing, 0.001f));
		s_gyroProcessing.smoothedYaw = rawYaw + (s_gyroProcessing.smoothedYaw - rawYaw) * smoothFactor;
		s_gyroProcessing.smoothedPitch = rawPitch + (s_gyroProcessing.smoothedPitch - rawPitch) * smoothFactor;

		outYaw = s_gyroProcessing.smoothedYaw * deltaTime * s_gyroConfig.sensitivity;
		outPitch = s_gyroProcessing.smoothedPitch * deltaTime * s_gyroConfig.sensitivity;
	}

	// ==========================================================
	// Processing - Touchpad (Internal)
	// ==========================================================

	inline void ProcessTouchpadMouse()
	{
		if (!s_pGamepad || !s_capabilities.hasTouchpad || !s_touchpadConfig.isEnabled)
			return;

		int numTouchpads = SDL_GetNumGamepadTouchpads(s_pGamepad);

		for (int tp = 0; tp < numTouchpads && tp < MAX_TOUCHPADS; tp++)
		{
			int numFingers = SDL_GetNumGamepadTouchpadFingers(s_pGamepad, tp);

			for (int finger = 0; finger < numFingers && finger < MAX_FINGERS; finger++)
			{
				bool fingerDown = false;
				float x = 0.0f, y = 0.0f, pressure = 0.0f;

				if (SDL_GetGamepadTouchpadFinger(s_pGamepad, tp, finger, &fingerDown, &x, &y, &pressure))
				{
					auto& state = s_touchpadFinger[tp][finger];

					if (fingerDown)
					{
						if (state.wasDown)
						{
							float deltaX = (x - state.lastX) * s_touchpadConfig.currentWidth;
							float deltaY = (y - state.lastY) * s_touchpadConfig.currentHeight;

							if (deltaX != 0.0f || deltaY != 0.0f)
							{
								INPUT input = {};
								input.type = INPUT_MOUSE;
								input.mi.dwFlags = MOUSEEVENTF_MOVE;
								input.mi.dx = static_cast<LONG>(deltaX);
								input.mi.dy = static_cast<LONG>(deltaY);
								SendInput(1, &input, sizeof(INPUT));
							}
						}

						state.lastX = x;
						state.lastY = y;
						state.wasDown = true;
					}
					else
					{
						state.wasDown = false;
					}
				}
			}
		}
	}

	static void ProcessTouchpadClick()
	{
		if (!s_pGamepad || !s_capabilities.hasTouchpad || !s_touchpadConfig.isEnabled)
			return;

		bool isPressed = SDL_GetGamepadButton(s_pGamepad, SDL_GAMEPAD_BUTTON_TOUCHPAD);

		if (isPressed && !s_wasTouchpadPressed[0])
		{
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			SendInput(1, &input, sizeof(INPUT));
		}
		else if (!isPressed && s_wasTouchpadPressed[0])
		{
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			SendInput(1, &input, sizeof(INPUT));
		}

		s_wasTouchpadPressed[0] = isPressed;
	}

	inline void ReleaseTouchpadClick()
	{
		for (int i = 0; i < MAX_TOUCHPADS; i++)
		{
			if (s_wasTouchpadPressed[i])
			{
				INPUT input = {};
				input.type = INPUT_MOUSE;
				input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &input, sizeof(INPUT));
				s_wasTouchpadPressed[i] = false;
			}
		}
	}

	// ==========================================================
	// Event Processing
	// ==========================================================

	inline void OnGamepadConnected(SDL_JoystickID deviceId)
	{
		if (s_pGamepad)
			return;

		s_pGamepad = SDL_OpenGamepad(deviceId);
		if (s_pGamepad)
		{
			LoadGamepadCapabilities(s_pGamepad);
		}
	}

	inline void OnGamepadDisconnected(SDL_JoystickID deviceId)
	{
		if (!s_pGamepad)
			return;

		if (deviceId == SDL_GetGamepadID(s_pGamepad))
		{
			SDL_CloseGamepad(s_pGamepad);
			s_pGamepad = nullptr;
			ReleaseTouchpadClick();
			ResetControllerState();

			// Try to connect to another available gamepad
			int count = 0;
			SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
			if (gamepads)
			{
				for (int i = 0; i < count; i++)
				{
					s_pGamepad = SDL_OpenGamepad(gamepads[i]);
					if (s_pGamepad)
					{
						LoadGamepadCapabilities(s_pGamepad);
						break;
					}
				}

				SDL_free(gamepads);
			}
		}
	}

	inline void ProcessSDLEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_GAMEPAD_ADDED:
					OnGamepadConnected(event.gdevice.which);
					break;

				case SDL_EVENT_GAMEPAD_REMOVED:
					OnGamepadDisconnected(event.gdevice.which);
					break;
			}
		}
	}

	// ==========================================================
	// Button Mapping
	// ==========================================================

	struct ButtonMapping
	{
		SDL_GamepadButton sdlButton;
		WORD xinputMask;
	};

	// Buttons that don't change based on controller style
	inline constexpr ButtonMapping s_commonButtons[] =
	{
		{ SDL_GAMEPAD_BUTTON_DPAD_UP,        XINPUT_GAMEPAD_DPAD_UP },
		{ SDL_GAMEPAD_BUTTON_DPAD_DOWN,      XINPUT_GAMEPAD_DPAD_DOWN },
		{ SDL_GAMEPAD_BUTTON_DPAD_LEFT,      XINPUT_GAMEPAD_DPAD_LEFT },
		{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     XINPUT_GAMEPAD_DPAD_RIGHT },
		{ SDL_GAMEPAD_BUTTON_START,          XINPUT_GAMEPAD_START },
		{ SDL_GAMEPAD_BUTTON_BACK,           XINPUT_GAMEPAD_BACK },
		{ SDL_GAMEPAD_BUTTON_LEFT_STICK,     XINPUT_GAMEPAD_LEFT_THUMB },
		{ SDL_GAMEPAD_BUTTON_RIGHT_STICK,    XINPUT_GAMEPAD_RIGHT_THUMB },
		{ SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  XINPUT_GAMEPAD_LEFT_SHOULDER },
		{ SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER },
	};

	// Face buttons for standard layout (Xbox/PlayStation)
	inline constexpr ButtonMapping s_standardFaceButtons[] =
	{
		{ SDL_GAMEPAD_BUTTON_SOUTH, XINPUT_GAMEPAD_A },
		{ SDL_GAMEPAD_BUTTON_EAST,  XINPUT_GAMEPAD_B },
		{ SDL_GAMEPAD_BUTTON_WEST,  XINPUT_GAMEPAD_X },
		{ SDL_GAMEPAD_BUTTON_NORTH, XINPUT_GAMEPAD_Y },
	};

	// Face buttons for Nintendo layout (A/B and X/Y swapped)
	inline constexpr ButtonMapping s_nintendoFaceButtons[] =
	{
		{ SDL_GAMEPAD_BUTTON_EAST,  XINPUT_GAMEPAD_A },
		{ SDL_GAMEPAD_BUTTON_SOUTH, XINPUT_GAMEPAD_B },
		{ SDL_GAMEPAD_BUTTON_NORTH, XINPUT_GAMEPAD_X },
		{ SDL_GAMEPAD_BUTTON_WEST,  XINPUT_GAMEPAD_Y },
	};

	// ==========================================================
	// Main Poll Function
	// ==========================================================

	inline DWORD PollController(XINPUT_STATE* pState, bool InvertABXYButtons)
	{
		ProcessSDLEvents();
		UpdateFrameTiming();

		if (!s_pGamepad || !SDL_GamepadConnected(s_pGamepad))
		{
			return ERROR_DEVICE_NOT_CONNECTED;
		}

		ProcessGyro();
		ProcessTouchpadMouse();
		ProcessTouchpadClick();

		// ==========================================================
		// Buttons
		// ==========================================================

		WORD buttons = 0;

		for (const auto& mapping : s_commonButtons)
		{
			if (SDL_GetGamepadButton(s_pGamepad, mapping.sdlButton))
				buttons |= mapping.xinputMask;
		}

		const auto& faceButtons = (s_capabilities.style == GamepadStyle::Nintendo && InvertABXYButtons) ? s_nintendoFaceButtons : s_standardFaceButtons;
		for (const auto& mapping : faceButtons)
		{
			if (SDL_GetGamepadButton(s_pGamepad, mapping.sdlButton))
				buttons |= mapping.xinputMask;
		}

		pState->Gamepad.wButtons = buttons;

		// ==========================================================
		// Triggers (SDL: 0-32767 -> XInput: 0-255)
		// ==========================================================

		Sint16 leftTrigger = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
		Sint16 rightTrigger = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

		pState->Gamepad.bLeftTrigger = static_cast<BYTE>((leftTrigger * 255) / 32767);
		pState->Gamepad.bRightTrigger = static_cast<BYTE>((rightTrigger * 255) / 32767);

		// ==========================================================
		// Thumbsticks (SDL Y-axis is inverted compared to XInput)
		// ==========================================================

		auto SafeNegate = [](Sint16 value) -> SHORT
		{
			if (value == -32768)
				return 32767;
			return -value;
		};

		pState->Gamepad.sThumbLX = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_LEFTX);
		pState->Gamepad.sThumbLY = SafeNegate(SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_LEFTY));
		pState->Gamepad.sThumbRX = SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_RIGHTX);
		pState->Gamepad.sThumbRY = SafeNegate(SDL_GetGamepadAxis(s_pGamepad, SDL_GAMEPAD_AXIS_RIGHTY));

		return ERROR_SUCCESS;
	}

	// ==========================================================
	// Vibration
	// ==========================================================

	inline DWORD SetVibration(XINPUT_VIBRATION* pVibration)
	{
		if (!s_pGamepad || !SDL_GamepadConnected(s_pGamepad))
		{
			return ERROR_DEVICE_NOT_CONNECTED;
		}

		if (!pVibration)
		{
			return ERROR_SUCCESS;
		}

		SDL_RumbleGamepad(s_pGamepad, pVibration->wLeftMotorSpeed, pVibration->wRightMotorSpeed, 100);

		return ERROR_SUCCESS;
	}
};