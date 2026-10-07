#pragma once

#include "../../Globals.cpp"

// ==========================
// IncreasedEntityPersistence
// ==========================

safetyhook::InlineHook EnemyLifetimeManager_ResetPopLimit;

static int __fastcall EnemyLifetimeManager_ResetPopLimit_Hook(char* thisp, int, int bucketType, int size)
{
	// The game want to clean up the array
	if (size == 0)
	{
		return EnemyLifetimeManager_ResetPopLimit.thiscall<int>(thisp, bucketType, size);
	}

	if (bucketType == 0) // bodies
	{
		size = std::max(size, IncreasedEntityPersistenceBodies);
	}

	if (bucketType == 1) // limbs
	{
		size = std::max(size, IncreasedEntityPersistenceLimbs);
	}

	return EnemyLifetimeManager_ResetPopLimit.thiscall<int>(thisp, bucketType, size);
}

static void ApplyIncreasedEntityPersistence()
{
	if (!IncreasedEntityPersistence) return;

	DWORD addr_EnemyLifetimeManager_Ctor = GetAddress(Addr::EnemyLifetimeManager_Ctor);

	if (IncreasedEntityPersistenceBodies != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_EnemyLifetimeManager_Ctor + 0x34, IncreasedEntityPersistenceBodies);
		MemoryHelper::WriteMemory<int>(addr_EnemyLifetimeManager_Ctor + 0x7A, IncreasedEntityPersistenceBodies);
	}

	if (IncreasedEntityPersistenceLimbs != 0)
	{
		MemoryHelper::WriteMemory<uint8_t>(addr_EnemyLifetimeManager_Ctor + 0x86, IncreasedEntityPersistenceLimbs);
		MemoryHelper::WriteMemory<int>(addr_EnemyLifetimeManager_Ctor + 0x91, IncreasedEntityPersistenceLimbs);
	}

	EnemyLifetimeManager_ResetPopLimit = HookHelper::CreateHook((void*)GetAddress(Addr::EnemyLifetimeManager_ResetPopLimit), &EnemyLifetimeManager_ResetPopLimit_Hook);
}
