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

static int* g_hpfDeathFrameCount = nullptr;
static float* g_hpfFrameTimeScale = nullptr;
static float* g_hpfConstraintMass = nullptr;

static uintptr_t g_hpfRetImpulseDamper = 0;
static uintptr_t g_hpfRetErrorScaler = 0;
static uintptr_t g_hpfRetMassCapture = 0;
static uintptr_t g_hpfRetForceDamper = 0;
static uintptr_t g_hpfRetTimestepLimiter = 0;

static volatile float g_hpfK08 = 0.8f;
static volatile float g_hpfK20 = 20.0f;
static volatile float g_hpfK2 = 2.0f;
static volatile float g_hpfK05 = 0.5f;
static volatile float g_hpfK100 = 100.0f;

__declspec(align(16)) static volatile unsigned int g_hpfAbsMask[4] = { 0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu };

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

__declspec(naked) static void HavokStub_PhysicsImpulseDamper()
{
	__asm
	{
		push	eax
		lea		esp, [esp - 10h]
		movups[esp], xmm3
		movss	xmm3, dword ptr[esp + 0Ch]

		mov		eax, dword ptr[g_hpfDeathFrameCount]
		cmp		dword ptr[eax], 0
		jle		Threshold20

		comiss	xmm3, dword ptr[g_hpfK08]
		ja		Scale
		jmp		Restore

		Threshold20 :
		comiss	xmm3, dword ptr[g_hpfK20]
		jbe		Restore

		Scale :
		mov		eax, dword ptr[g_hpfFrameTimeScale]
		divss	xmm3, dword ptr[eax]
		movss	dword ptr[esp + 0Ch], xmm3

		Restore :
		movups	xmm3, [esp]
		lea		esp, [esp + 10h]
		pop		eax

		// 8 stolen bytes
		shufps	xmm2, xmm1, 0AAh
		shufps	xmm3, xmm3, 0FFh
		jmp		dword ptr[g_hpfRetImpulseDamper]
	}
}

__declspec(naked) static void HavokStub_ConstraintErrorScaler()
{
	__asm
	{
		push	eax
		lea		esp, [esp - 10h]
		movups[esp], xmm2
		movss	xmm2, dword ptr[esp + 0Ch]

		mov		eax, dword ptr[g_hpfFrameTimeScale]
		divss	xmm2, dword ptr[eax]
		movss	dword ptr[esp + 0Ch], xmm2

		movups	xmm2, [esp]
		lea		esp, [esp + 10h]
		pop		eax

		// 6 stolen bytes
		movaps	xmm2, xmmword ptr[esi]
		mulps	xmm1, xmm2
		jmp		dword ptr[g_hpfRetErrorScaler]
	}
}

__declspec(naked) static void HavokStub_ConstraintMassCapture()
{
	__asm
	{
		push	eax
		push	ecx

		mov		eax, dword ptr[esp + 18h]
		mov		ecx, dword ptr[g_hpfConstraintMass]
		mov		dword ptr[ecx], eax

		pop		ecx
		pop		eax

		// 6 stolen bytes
		fld		dword ptr[esp + 10h]
		_emit 0DEh
		_emit 0FAh
		jmp		dword ptr[g_hpfRetMassCapture]
	}
}

__declspec(naked) static void HavokStub_ConstraintForceDamper()
{
	__asm
	{
		push	ecx
		lea		esp, [esp - 10h]
		movups[esp], xmm0

		movss	xmm0, dword ptr[esp + 16Ch]
		andps	xmm0, xmmword ptr[g_hpfAbsMask]
		comiss	xmm0, dword ptr[g_hpfK08]
		jbe		Done

		movss	xmm0, dword ptr[esp + 170h]
		mov		ecx, dword ptr[g_hpfFrameTimeScale]
		divss	xmm0, dword ptr[ecx]
		mov		ecx, dword ptr[g_hpfConstraintMass]
		divss	xmm0, dword ptr[ecx]
		mulss	xmm0, dword ptr[g_hpfK08]
		movss	dword ptr[eax + 0Ch], xmm0

		Done :
		movups	xmm0, [esp]
		lea		esp, [esp + 10h]
		pop		ecx

		// 6 stolen bytes
		fld		dword ptr[esp + 68h]
		fchs
		jmp		dword ptr[g_hpfRetForceDamper]
	}
}

__declspec(naked) static void HavokStub_TimestepLimiter()
{
	__asm
	{
		push	ecx
		lea		esp, [esp - 10h]
		movups[esp], xmm0

		movss	xmm0, dword ptr[esp + 16Ch]
		andps	xmm0, xmmword ptr[g_hpfAbsMask]

		comiss	xmm0, dword ptr[g_hpfK2]
		ja		Clamp

		comiss	xmm0, dword ptr[g_hpfK05]
		jbe		Done

		mov		ecx, dword ptr[g_hpfConstraintMass]
		movss	xmm0, dword ptr[ecx]
		comiss	xmm0, dword ptr[g_hpfK100]
		jb		Done

		Clamp :
		mov		dword ptr[esp + 24h], 0C1F00000h // -30.0f

		Done :
		movups	xmm0, [esp]
		lea		esp, [esp + 10h]
		pop		ecx

		// 7 stolen bytes
		fld		dword ptr[esp + 158h]
		jmp		dword ptr[g_hpfRetTimestepLimiter]
	}
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
	DWORD addr_constraintMassCapture = ScanModuleSignature(g_State.GameModule, "D9 44 24 10 DE FA D9 C9 D9 58 0C D9 44 24 68 D9 E0 D9 5C 24 10 D9 84 24 58 01 00 00", "constraintMassCapture");

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

	g_hpfDeathFrameCount = &g_State.deathFrameCount;
	g_hpfFrameTimeScale = &g_State.frameTimeScale;
	g_hpfConstraintMass = &g_State.constraintMass;

	g_hpfRetImpulseDamper = addr_physicsImpulseDamper + 8;
	g_hpfRetErrorScaler = addr_constraintErrorScaler + 6;
	g_hpfRetMassCapture = addr_constraintMassCapture + 6;
	g_hpfRetForceDamper = (addr_constraintMassCapture + 0xB) + 6;
	g_hpfRetTimestepLimiter = (addr_constraintMassCapture + 0x15) + 7;

	MemoryHelper::MakeJMP(addr_physicsImpulseDamper, reinterpret_cast<uintptr_t>(&HavokStub_PhysicsImpulseDamper));
	MemoryHelper::MakeNOP(addr_physicsImpulseDamper + 5, 3);

	MemoryHelper::MakeJMP(addr_constraintErrorScaler, reinterpret_cast<uintptr_t>(&HavokStub_ConstraintErrorScaler));
	MemoryHelper::MakeNOP(addr_constraintErrorScaler + 5, 1);

	MemoryHelper::MakeJMP(addr_constraintMassCapture, reinterpret_cast<uintptr_t>(&HavokStub_ConstraintMassCapture));
	MemoryHelper::MakeNOP(addr_constraintMassCapture + 5, 1);

	MemoryHelper::MakeJMP(addr_constraintMassCapture + 0xB, reinterpret_cast<uintptr_t>(&HavokStub_ConstraintForceDamper));
	MemoryHelper::MakeNOP(addr_constraintMassCapture + 0xB + 5, 1);

	MemoryHelper::MakeJMP(addr_constraintMassCapture + 0x15, reinterpret_cast<uintptr_t>(&HavokStub_TimestepLimiter));
	MemoryHelper::MakeNOP(addr_constraintMassCapture + 0x15 + 5, 2);
}