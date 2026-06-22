#pragma once

#include "../../Globals.cpp"

// =========================
// HavokPhysicsFix
// =========================

safetyhook::InlineHook InitializePhysicsSolverParameters;
safetyhook::InlineHook Build1DAngularConstraintJacobian;
safetyhook::InlineHook SolveBallSocketChainConstraints;
safetyhook::InlineHook BuildContactConstraintJacobian;
safetyhook::InlineHook ProcessEntityDeath;
safetyhook::InlineHook MainLoop;

static void __cdecl BuildContactConstraintJacobian_Hook(__m128* a1, float* a2, bool a3, __m128** a4)
{
	float backup_deltaTime = a2[3];
	float backup_friction = a2[9];

	float timeScale = TARGET_FRAME_TIME / backup_deltaTime;
	a2[3] = backup_deltaTime / timeScale;
	a2[9] = backup_friction / timeScale;

	BuildContactConstraintJacobian.unsafe_call<void>(a1, a2, a3, a4);

	a2[3] = backup_deltaTime;
	a2[9] = backup_friction;
}

static void __cdecl SolveBallSocketChainConstraints_Hook(float* cons, __m128* a2, __m128* a3, __m128** a4)
{
	float rhs_bak = cons[7];
	cons[7] /= g_State.frameTimeScale;
	SolveBallSocketChainConstraints.unsafe_call<void>(cons, a2, a3, a4);
	cons[7] = rhs_bak;
}

static void __cdecl Build1DAngularConstraintJacobian_Hook(__m128* a1, float* cons, __m128** a3)
{
	float rhs_bak = cons[7];
	cons[7] /= g_State.frameTimeScale;
	Build1DAngularConstraintJacobian.unsafe_call<void>(a1, cons, a3);
	cons[7] = rhs_bak;
}

static int __fastcall InitializePhysicsSolverParameters_Hook(float* thisp, int, float* a2, float* a3)
{
	g_State.frameTimeScale = TARGET_FRAME_TIME / a3[2];
	return InitializePhysicsSolverParameters.unsafe_thiscall<int>(thisp, a2, a3);
}

static char __fastcall ProcessEntityDeath_Hook(DWORD* thisp, int, int a2, float a3, int a4, int a5, int a6)
{
	// Ragdoll Death?
	if (a5 == 22)
	{
		g_State.deathFrameCount = 5;
	}

	return ProcessEntityDeath.unsafe_thiscall<char>(thisp, a2, a3, a4, a5, a6);
}

static int __cdecl MainLoop_Hook()
{
	if (g_State.deathFrameCount != 0)
	{
		g_State.deathFrameCount--;
	}

	if (RawMouseInput)
	{
		g_State.frameRawX = g_State.rawMouseDeltaX.exchange(0);
		g_State.frameRawY = g_State.rawMouseDeltaY.exchange(0);
	}

	if (AchievementSupport)
	{
		AchievementOverlay::Update(GetD3D9Device());
	}

	return MainLoop.unsafe_ccall<int>();
}
static void ApplyHavokPhysicsFix()
{
	if (!HavokPhysicsFix) return;

	DWORD addr_BuildContactConstraintJacobian = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC A4 00 00 00 8B 55 08", "BuildContactConstraintJacobian");
	DWORD addr_SolveBallSocketChainConstraints = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 81 EC 84 00 00 00 F3 0F 10 41 04", "SolveBallSocketChainConstraints");
	DWORD addr_Build1DAngularConstraintJacobian = ScanModuleSignature(g_State.GameModule, "55 8B EC 83 E4 F0 83 EC 14 8B 45 08 53 8B 5D 10", "Build1DAngularConstraintJacobian");
	DWORD addr_InitializePhysicsSolverParameters = ScanModuleSignature(g_State.GameModule, "8B 44 24 04 D9 80 0C 01 00 00 D9 19", "InitializePhysicsSolverParameters");
	DWORD addr_ProcessEntityDeath = ScanModuleSignature(g_State.GameModule, "53 56 8B F1 8B 4C 24 18 8B C1 32 DB 83 E8 16 0F 84 17 01 00 00", "ProcessEntityDeath");
	DWORD addr_physicsImpulseDamper = ScanModuleSignature(g_State.GameModule, "0F C6 D1 AA 0F C6 DB FF F3 0F 58 D4 0F 28 C8 0F C6 C8 FF F3 0F 5C CA F3 0F 59 CB 0F 28 E1", "physicsImpulseDamper");
	DWORD addr_constraintErrorScaler = ScanModuleSignature(g_State.GameModule, "0F 28 16 0F 59 CA 0F 58 C3 0F 58 C1 8D 4A 10 0F 28 C8 0F C6 C8 55 F3 0F 58 C8 83 C2 20 0F C6 C0 AA F3 0F 58 C1 0F C6 D2 FF F3 0F 5C D0 F3 0F 11 94 24 30 02 00 00", "constraintErrorScaler");
	DWORD addr_constraintMassCapture = ScanModuleSignature(g_State.GameModule, "D9 44 24 10 DE FA D9 C9 D9 58 0C D9 44 24 68", "constraintMassCapture");

	if (addr_BuildContactConstraintJacobian == 0 ||
		addr_SolveBallSocketChainConstraints == 0 ||
		addr_Build1DAngularConstraintJacobian == 0 ||
		addr_InitializePhysicsSolverParameters == 0 ||
		addr_ProcessEntityDeath == 0 ||
		addr_physicsImpulseDamper == 0 ||
		addr_constraintErrorScaler == 0 ||
		addr_constraintMassCapture == 0) {
		return;
	}

	BuildContactConstraintJacobian = HookHelper::CreateHook((void*)addr_BuildContactConstraintJacobian, &BuildContactConstraintJacobian_Hook);
	SolveBallSocketChainConstraints = HookHelper::CreateHook((void*)addr_SolveBallSocketChainConstraints, &SolveBallSocketChainConstraints_Hook);
	Build1DAngularConstraintJacobian = HookHelper::CreateHook((void*)addr_Build1DAngularConstraintJacobian, &Build1DAngularConstraintJacobian_Hook);
	InitializePhysicsSolverParameters = HookHelper::CreateHook((void*)addr_InitializePhysicsSolverParameters, &InitializePhysicsSolverParameters_Hook);
	ProcessEntityDeath = HookHelper::CreateHook((void*)addr_ProcessEntityDeath, &ProcessEntityDeath_Hook);

	static SafetyHookMid physicsImpulseDamper{};
	physicsImpulseDamper = safetyhook::create_mid(addr_physicsImpulseDamper,
		[](safetyhook::Context& ctx)
		{
			if (g_State.deathFrameCount > 0 && ctx.xmm3.f32[3] > 0.8f)
			{
				ctx.xmm3.f32[3] = ctx.xmm3.f32[3] / g_State.frameTimeScale;
			}
			else if (ctx.xmm3.f32[3] > 20.0f)
			{
				ctx.xmm3.f32[3] = ctx.xmm3.f32[3] / g_State.frameTimeScale;
			}
		}
	);

	static SafetyHookMid constraintErrorScaler{};
	constraintErrorScaler = safetyhook::create_mid(addr_constraintErrorScaler,
		[](safetyhook::Context& ctx)
		{
			ctx.xmm2.f32[3] = ctx.xmm2.f32[3] / g_State.frameTimeScale;
		}
	);

	static SafetyHookMid constraintMassCapture{};
	constraintMassCapture = safetyhook::create_mid(addr_constraintMassCapture,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp = ctx.esp;
			float* v306_ptr = (float*)(esp + 0x10);
			g_State.constraintMass = *v306_ptr;
		}
	);

	static SafetyHookMid constraintForceDamper{};
	constraintForceDamper = safetyhook::create_mid(addr_constraintMassCapture + 0xB,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp = ctx.esp;
			uint32_t eax = ctx.eax;

			float* v369_ptr = (float*)(esp + 0x158);
			float* v370_ptr = (float*)(esp + 0x15C);

			if (fabs(*v369_ptr) > 0.8f)
			{
				float* tau_ptr = (float*)(eax + 0x0C);
				float scaleFactor = 0.8f;

				*tau_ptr = ((*v370_ptr / g_State.frameTimeScale) / g_State.constraintMass) * scaleFactor;
			}
		}
	);

	static SafetyHookMid timestepLimiter{};
	timestepLimiter = safetyhook::create_mid(addr_constraintMassCapture + 0x15,
		[](safetyhook::Context& ctx)
		{
			uint32_t esp_val = ctx.esp;

			float* v307 = (float*)(esp_val + 0x10);
			float* v369 = (float*)(esp_val + 0x158);

			if (fabs(*v369) > 2.0f || (fabs(*v369) > 0.5f && g_State.constraintMass >= 100.0f))
			{
				float* v307 = (float*)(esp_val + 0x10);
				*v307 = -30.0f;
			}
		}
	);
}
