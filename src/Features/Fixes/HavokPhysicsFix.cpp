#pragma once

#include "../../Globals.cpp"

#include <emmintrin.h>

// =========================
// HavokPhysicsFix
// =========================

safetyhook::InlineHook hkpWorld_stepDeltaTime;
safetyhook::InlineHook hkpConstraintSolverSetup_solve;
safetyhook::InlineHook hkpConstraintSolverSetup_oneStepIntegrate;
safetyhook::InlineHook hkRigidMotionUtilApplyForcesAndStep;
safetyhook::InlineHook hkpContinuousSimulation_simulateToi;
safetyhook::InlineHook hkpContinuousSimulation_collideIslandNarrowPhaseContinuous;
safetyhook::InlineHook hkpWorldCallbackUtil_fireContactPointAdded;
safetyhook::InlineHook HavokManager_CloseHavok;
safetyhook::InlineHook PlayerFireTKSM_ApplyKeyframeToTarget;
safetyhook::InlineHook hkpMotion_setLinearVelocity;
safetyhook::InlineHook hkpMotion_setAngularVelocity;
safetyhook::InlineHook hkpMotion_applyLinearImpulse;

struct hkVector4
{
	float x, y, z, w;
};

struct hkStepInfo
{
	float m_startTime;
	float m_endTime;
	float m_deltaTime;
	float m_invDeltaTime;
};

struct hkpSolverInfo
{
	uint8_t pad0[0x10];
	hkVector4 m_globalAccelerationPerSubStep{};
	hkVector4 m_globalAccelerationPerStep{};
	uint8_t pad30[0xDC];
	float m_deltaTime;
	float m_invDeltaTime;
	int m_numSteps;
	int m_numMicroSteps;
	float m_invNumMicroSteps;
	float m_invNumSteps;
	bool m_forceCoherentConstraintOrderingInSolver;
	uint8_t m_deactivationNumInactiveFramesSelectFlag[2];
	uint8_t m_deactivationIntegrateCounter;
	uint8_t pad128[0x8];
};

// The motion of an hkpEntity, with its hkMotionState and hkSweptTransform members inline
struct hkpMotion
{
	uint8_t pad0[0x8];
	uint8_t m_type;
	uint8_t pad9[0x7];
	hkVector4 m_transform[4]{}; // Rotation columns then translation
	hkVector4 m_centerOfMass0{}; // w = Start time of the sweep
	hkVector4 m_centerOfMass1{}; // w = Inverse duration of the sweep
	hkVector4 m_rotation0{};
	hkVector4 m_rotation1{};
	hkVector4 m_centerOfMassLocal{};
	hkVector4 m_deltaAngle{};
	uint8_t padB0[0x20];
	hkVector4 m_linearVelocity{};
	hkVector4 m_angularVelocity{};
	uint8_t padF0[0x30];
};

struct hkContactPoint
{
	hkVector4 m_position{};
	hkVector4 m_separatingNormal{}; // w = Distance
};

struct hkArray
{
	uintptr_t* m_data;
	int m_size;
	int m_capacityAndFlags;
};

// hkpWorld
static constexpr uintptr_t OFF_WORLD_GRAVITY = 0x10;
static constexpr uintptr_t OFF_WORLD_ACTIVE_SIMULATION_ISLANDS = 0x28;
static constexpr uintptr_t OFF_WORLD_INACTIVE_SIMULATION_ISLANDS = 0x34;
static constexpr uintptr_t OFF_WORLD_COLLISION_INPUT = 0x74;

// hkpSimulationIsland and hkpEntity
static constexpr uintptr_t OFF_ISLAND_ENTITIES = 0x48;
static constexpr uintptr_t OFF_ENTITY_MOTION = 0xE0;

// hkpToiEvent, the two entities of the contact
static constexpr uintptr_t OFF_TOI_EVENT_ENTITIES = 0xC;

// hkpCdBody and hkpCollidable
static constexpr uintptr_t OFF_CD_BODY_PARENT = 0xC;
static constexpr uintptr_t OFF_COLLIDABLE_OWNER_OFFSET = 0x10;

// PlayerFireTKSM
static constexpr uintptr_t OFF_FIRE_TK_TARGET_BODY = 0xC0;
static constexpr uintptr_t OFF_FIRE_TK_ACCUMULATOR = 0xD0;

// hkpContactPointAddedEvent
static constexpr uintptr_t OFF_POINT_ADDED_BODY_A = 0x0;
static constexpr uintptr_t OFF_POINT_ADDED_BODY_B = 0x4;
static constexpr uintptr_t OFF_POINT_ADDED_TYPE = 0x8;
static constexpr uintptr_t OFF_POINT_ADDED_CONTACT_POINT = 0x10;
static constexpr uintptr_t OFF_POINT_ADDED_PROJECTED_VELOCITY = 0x1C;
static constexpr uintptr_t OFF_TOI_POINT_ADDED_TOI = 0x30; // hkpToiPointAddedEvent
static constexpr int POINT_ADDED_TYPE_TOI = 0;

// hkpCollisionDispatcher, through the world
static constexpr uintptr_t OFF_WORLD_COLLISION_DISPATCHER = 0x7C;
static constexpr uintptr_t OFF_DISPATCHER_COLLISION_QUALITY_INFO = 0x1C20;
static constexpr uintptr_t SIZE_COLLISION_QUALITY_INFO = 0x40;
static constexpr int NUM_COLLISION_QUALITY_INFOS = 8;

// hkpCollisionQualityInfo: m_minSafeDeltaTime, m_minAbsoluteSafeDeltaTime and m_minToiDeltaTime
static constexpr uintptr_t OFF_COLLISION_QUALITY_TIMES[] = { 0x20, 0x24, 0x38 };

// hkpMotion::MotionType
static constexpr uint8_t MOTION_KEYFRAMED = 6;

static constexpr float KEYFRAMED_JUMP_DISTANCE = 1.0f;

struct SimulatedBody
{
	hkVector4 prevCenterOfMass{};
	hkVector4 prevRotation{};
	hkVector4 centerOfMass{};
	hkVector4 rotation{};
	hkVector4 transform[4];
	hkVector4 shownTransform[4];
	hkVector4 shownCenterOfMass{};
	hkVector4 shownRotation{};
	uint32_t tickedStep = 0;
	uint32_t seenStep = 0;
	bool ticked = false;
	bool interpolate = false;
	bool shown = false;
};

struct KeyframedBody
{
	hkVector4 tickCenterOfMass{};
	hkVector4 tickRotation{};
	hkVector4 centerOfMass{};
	hkVector4 rotation{};
	hkVector4 linearVelocity{};
	hkVector4 angularVelocity{};
	hkVector4 prevLinearVelocity{};
	hkVector4 prevAngularVelocity{};
	hkVector4 pathCenterOfMass{};
	hkVector4 pathRotation{};
	float pathTime = 0.0f;
	uint32_t seenStep = 0;
	hkpMotion sweptMotion{};
	uint32_t sweptStep = 0;
};

struct SavedMotion
{
	uintptr_t entity;
	hkpMotion motion;
};

struct KinesisControl
{
	uintptr_t fireState = 0;
	uintptr_t targetBody = 0;
	uint8_t tickState[0x30] = {};
	uint8_t pendingState[0x30] = {};
	uint8_t prevPendingState[0x30] = {};
	int pendingType = -1;
	bool hasPendingState = false;
	bool hasPrevPendingState = false;
	bool isCapturing = false;
};

struct KinesisCommand
{
	hkVector4 linearVelocity{};
	hkVector4 angularVelocity{};
	hkVector4 prevLinearVelocity{};
	hkVector4 prevAngularVelocity{};
	int type = -1;
};

struct CapturedVelocity
{
	uintptr_t entity;
	hkVector4 linearVelocity{};
	hkVector4 angularVelocity{};
};

enum class IslandStep
{
	Frame,
	Frozen,
	Tick
};

static void(__thiscall* hkpConstraintQueryIn_set)(void*, const hkpSolverInfo*, const hkStepInfo*) = nullptr;
static void(__cdecl* hkpEntityAabbUtil_entityBatchRecalcAabb)(uintptr_t, const uintptr_t*, int) = nullptr;

static uintptr_t g_havokWorld = 0;
static uint32_t g_stepCount = 0;
static bool g_isWorldStep = false;
static bool g_isTickStep = false;
static float g_tickLength = TARGET_FRAME_TIME;
static float g_tickStart = 0.0f;
static float g_tickAccumulator = 0.0f;
static float g_kinesisBlend = 1.0f;
static float g_collisionTimeScale = 1.0f;
static float g_frameDeltaTime = 0.0f;
static float g_savedCollisionQualityTimes[NUM_COLLISION_QUALITY_INFOS][std::size(OFF_COLLISION_QUALITY_TIMES)] = {};

static bool g_isTickPrepared = false;
static bool g_isIslandCollide = false;
static uint32_t g_keyframedSeenStep = 0;
static bool g_isTickQueryPrepared = false;
static bool g_hasTickDeactivation = false;
static uint8_t g_tickDeactivationCounter = 0;
static uint8_t g_tickDeactivationSelectFlags[2] = {};

// Havok reads these with aligned loads
alignas(16) static hkStepInfo g_tickStepInfo = {};
alignas(16) static hkpSolverInfo g_tickSolverInfo = {};
alignas(16) static uint8_t g_tickConstraintQuery[0x60] = {};

struct EntityHash
{
	size_t operator()(uintptr_t entity) const noexcept
	{
		return (entity >> 4) ^ (entity >> 12);
	}
};

static std::unordered_map<uintptr_t, SimulatedBody, EntityHash> g_simulatedBodies;
static std::unordered_map<uintptr_t, KeyframedBody, EntityHash> g_keyframedBodies;
static KinesisControl g_kinesis;
static std::unordered_map<uintptr_t, KinesisCommand, EntityHash> g_kinesisCommands;
static std::vector<CapturedVelocity> g_kinesisCaptures;
static std::vector<uintptr_t> g_keyframedEntities;
static std::vector<SavedMotion> g_savedMotions;
static std::vector<SavedMotion> g_collideMotions;
static std::vector<uintptr_t> g_toiTickedEntities;
static std::vector<uintptr_t> g_toiFrameEntities;
static int g_shownBodyCount = 0;

static hkpMotion* GetMotion(uintptr_t entity)
{
	return reinterpret_cast<hkpMotion*>(entity + OFF_ENTITY_MOTION);
}

static bool IsDynamicMotion(uint8_t type)
{
	// Dynamic, sphere, stabilized sphere, box, stabilized box and thin box inertia
	return (type >= 1 && type <= 5) || type == 8;
}

static bool IsSimulatedInTicks(uintptr_t entity)
{
	auto it = g_simulatedBodies.find(entity);
	return it != g_simulatedBodies.end() && it->second.tickedStep == g_stepCount;
}

static bool IsTickedThisStep(uintptr_t entity)
{
	auto it = g_simulatedBodies.find(entity);
	return it != g_simulatedBodies.end() && it->second.tickedStep == g_stepCount && it->second.ticked;
}

// Stops at the first entity func returns false for
template <typename Func>
static bool ForEachIslandEntityWhile(uintptr_t islandsOffset, Func&& func)
{
	const hkArray& islands = *reinterpret_cast<hkArray*>(g_havokWorld + islandsOffset);

	for (int i = 0; i < islands.m_size; i++)
	{
		const hkArray& entities = *reinterpret_cast<hkArray*>(islands.m_data[i] + OFF_ISLAND_ENTITIES);

		for (int j = 0; j < entities.m_size; j++)
		{
			if (!func(entities.m_data[j])) return false;
		}
	}

	return true;
}

template <typename Func>
static void ForEachIslandEntity(uintptr_t islandsOffset, Func&& func)
{
	const hkArray& islands = *reinterpret_cast<hkArray*>(g_havokWorld + islandsOffset);

	for (int i = 0; i < islands.m_size; i++)
	{
		const hkArray& entities = *reinterpret_cast<hkArray*>(islands.m_data[i] + OFF_ISLAND_ENTITIES);

		for (int j = 0; j < entities.m_size; j++)
		{
			func(entities.m_data[j]);
		}
	}
}

static inline __m128 LoadVector(const hkVector4& v)
{
	return _mm_loadu_ps(&v.x);
}

static inline hkVector4 StoreVector(__m128 v)
{
	hkVector4 result;
	_mm_storeu_ps(&result.x, v);
	return result;
}

static inline __m128 MaskXyz()
{
	return _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
}

static inline __m128 SignBits()
{
	return _mm_castsi128_ps(_mm_set1_epi32(static_cast<int>(0x80000000u)));
}

// (x + y) + z
static inline float Sum3(__m128 v)
{
	__m128 sum = _mm_add_ss(v, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)));
	return _mm_cvtss_f32(_mm_add_ss(sum, _mm_movehl_ps(v, v)));
}

// ((x + y) + z) + w
static inline float Sum4(__m128 v)
{
	__m128 sum = _mm_add_ss(v, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)));
	sum = _mm_add_ss(sum, _mm_movehl_ps(v, v));
	return _mm_cvtss_f32(_mm_add_ss(sum, _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3))));
}

static inline float SquareRoot(float value)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
}

static inline __m128 LerpVector(__m128 a, __m128 b, __m128 t)
{
	return _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t));
}

// memcmp(a, b, 16) == 0, and the same for the first 12 bytes
static inline bool BitsEqual16(const void* a, const void* b)
{
	__m128i equal = _mm_cmpeq_epi32(_mm_loadu_si128(static_cast<const __m128i*>(a)), _mm_loadu_si128(static_cast<const __m128i*>(b)));
	return _mm_movemask_epi8(equal) == 0xFFFF;
}

static inline bool BitsEqual12(const void* a, const void* b)
{
	__m128i equal = _mm_cmpeq_epi32(_mm_loadu_si128(static_cast<const __m128i*>(a)), _mm_loadu_si128(static_cast<const __m128i*>(b)));
	return (_mm_movemask_epi8(equal) & 0x0FFF) == 0x0FFF;
}

static hkVector4 Add(const hkVector4& a, const hkVector4& b)
{
	return StoreVector(_mm_add_ps(LoadVector(a), LoadVector(b)));
}

static hkVector4 Sub(const hkVector4& a, const hkVector4& b)
{
	return StoreVector(_mm_sub_ps(LoadVector(a), LoadVector(b)));
}

static float Length3(const hkVector4& v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static hkVector4 Scale(const hkVector4& v, float scale)
{
	return { v.x * scale, v.y * scale, v.z * scale, 0.0f };
}

static hkVector4 ClampLength(const hkVector4& v, float maxLength)
{
	float length = Length3(v);
	return length > maxLength ? Scale(v, maxLength / length) : hkVector4{ v.x, v.y, v.z, 0.0f };
}

static hkVector4 Lerp(const hkVector4& a, const hkVector4& b, float t)
{
	return StoreVector(LerpVector(LoadVector(a), LoadVector(b), _mm_set1_ps(t)));
}

static hkVector4 Nlerp(const hkVector4& a, const hkVector4& b, float t)
{
	__m128 va = LoadVector(a);
	__m128 vb = LoadVector(b);

	// Shortest path between the two rotations
	float dot = Sum4(_mm_mul_ps(va, vb));
	__m128 q = LerpVector(va, dot < 0.0f ? _mm_xor_ps(vb, SignBits()) : vb, _mm_set1_ps(t));

	float length = SquareRoot(Sum4(_mm_mul_ps(q, q)));
	if (length < 1e-6f) return b;

	return StoreVector(_mm_div_ps(q, _mm_set1_ps(length)));
}

// Rotation from a to b as its axis scaled by its angle
static hkVector4 GetRotationDelta(const hkVector4& a, const hkVector4& b)
{
	__m128 va = LoadVector(a);
	__m128 vb = LoadVector(b);

	// b * conjugate(a), the shortest way: a.w * b - b.w * a - (b.yzx * a.zxy - b.zxy * a.yzx) for the axis
	__m128 awTimesB = _mm_mul_ps(_mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 3, 3, 3)), vb);
	__m128 bwTimesA = _mm_mul_ps(_mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 3, 3, 3)), va);
	__m128 crossLeft = _mm_mul_ps(_mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2)));
	__m128 crossRight = _mm_mul_ps(_mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1)));
	__m128 axis = _mm_sub_ps(_mm_sub_ps(awTimesB, bwTimesA), _mm_sub_ps(crossLeft, crossRight));

	// a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
	__m128 products = _mm_mul_ps(va, vb);
	__m128 w = _mm_add_ss(_mm_shuffle_ps(products, products, _MM_SHUFFLE(3, 3, 3, 3)), products);
	w = _mm_add_ss(w, _mm_shuffle_ps(products, products, _MM_SHUFFLE(1, 1, 1, 1)));
	float qw = _mm_cvtss_f32(_mm_add_ss(w, _mm_movehl_ps(products, products)));

	if (qw < 0.0f)
	{
		axis = _mm_xor_ps(axis, SignBits());
		qw = -qw;
	}

	float length = SquareRoot(Sum3(_mm_mul_ps(axis, axis)));
	float scale = length < 1e-6f ? 2.0f : 2.0f * atan2f(length, qw) / length;

	return StoreVector(_mm_and_ps(_mm_mul_ps(axis, _mm_set1_ps(scale)), MaskXyz()));
}

// Same as hkSweptTransform::approxTransformAt, the transform of the body at its interpolated center of mass and rotation
static void BuildTransform(hkVector4* transform, const hkVector4& centerOfMass, const hkVector4& q, const hkVector4& centerOfMassLocal)
{
	hkVector4& x = transform[0];
	hkVector4& y = transform[1];
	hkVector4& z = transform[2];

	x.x = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	x.y = 2.0f * (q.x * q.y + q.w * q.z);
	x.z = 2.0f * (q.x * q.z - q.w * q.y);

	y.x = 2.0f * (q.x * q.y - q.w * q.z);
	y.y = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
	y.z = 2.0f * (q.y * q.z + q.w * q.x);

	z.x = 2.0f * (q.x * q.z + q.w * q.y);
	z.y = 2.0f * (q.y * q.z - q.w * q.x);
	z.z = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

	const hkVector4& l = centerOfMassLocal;
	transform[3].x = centerOfMass.x - (x.x * l.x + y.x * l.y + z.x * l.z);
	transform[3].y = centerOfMass.y - (x.y * l.x + y.y * l.y + z.y * l.z);
	transform[3].z = centerOfMass.z - (x.z * l.x + y.z * l.y + z.z * l.z);
}

static uintptr_t GetCdBodyEntity(uintptr_t cdBody)
{
	// The collidable at the root of the body belongs to the entity
	while (uintptr_t parent = *reinterpret_cast<uintptr_t*>(cdBody + OFF_CD_BODY_PARENT))
	{
		cdBody = parent;
	}

	return cdBody + *reinterpret_cast<int8_t*>(cdBody + OFF_COLLIDABLE_OWNER_OFFSET);
}

// Velocity of the body at the point, with its center of mass along its sweep at the time
static hkVector4 GetPointVelocity(uintptr_t entity, const hkVector4& point, float time)
{
	const hkpMotion* motion = GetMotion(entity);

	float t = std::clamp((time - motion->m_centerOfMass0.w) * motion->m_centerOfMass1.w, 0.0f, 1.0f);
	hkVector4 centerOfMass = Lerp(motion->m_centerOfMass0, motion->m_centerOfMass1, t);
	hkVector4 r = { point.x - centerOfMass.x, point.y - centerOfMass.y, point.z - centerOfMass.z, 0.0f };

	const hkVector4& v = motion->m_linearVelocity;
	const hkVector4& w = motion->m_angularVelocity;

	return { v.x + w.y * r.z - w.z * r.y, v.y + w.z * r.x - w.x * r.z, v.z + w.x * r.y - w.y * r.x, 0.0f };
}

enum class SweepTime
{
	None, // Not moving this step
	Frame, // Its motion in the frame's time
	Tick // A tick's motion in the frame's time
};

static SweepTime GetSweepTime(uintptr_t entity)
{
	const hkpMotion* motion = GetMotion(entity);

	if (motion->m_centerOfMass1.w == 0.0f || (BitsEqual12(&motion->m_centerOfMass0, &motion->m_centerOfMass1) && motion->m_deltaAngle.w == 0.0f))
	{
		return SweepTime::None;
	}

	if (g_isTickStep)
	{
		if (IsTickedThisStep(entity)) return SweepTime::Tick;

		if (g_isIslandCollide && std::any_of(g_collideMotions.begin(), g_collideMotions.end(), [entity](const SavedMotion& saved) { return saved.entity == entity; }))
		{
			return SweepTime::Tick;
		}
	}

	return SweepTime::Frame;
}

static void FixToiProjectedVelocity(uintptr_t event)
{
	uintptr_t entityA = GetCdBodyEntity(*reinterpret_cast<uintptr_t*>(event + OFF_POINT_ADDED_BODY_A));
	uintptr_t entityB = GetCdBodyEntity(*reinterpret_cast<uintptr_t*>(event + OFF_POINT_ADDED_BODY_B));
	SweepTime timeA = GetSweepTime(entityA);
	SweepTime timeB = GetSweepTime(entityB);
	float& projectedVelocity = *reinterpret_cast<float*>(event + OFF_POINT_ADDED_PROJECTED_VELOCITY);

	if (timeA != SweepTime::Tick && timeB != SweepTime::Tick) return;

	if (timeA != SweepTime::Frame && timeB != SweepTime::Frame)
	{
		projectedVelocity *= g_frameDeltaTime / g_tickLength;
		return;
	}

	const hkContactPoint* cp = *reinterpret_cast<const hkContactPoint**>(event + OFF_POINT_ADDED_CONTACT_POINT);
	float toi = *reinterpret_cast<float*>(event + OFF_TOI_POINT_ADDED_TOI);
	hkVector4 velocityA = GetPointVelocity(entityA, cp->m_position, toi);
	hkVector4 velocityB = GetPointVelocity(entityB, cp->m_position, toi);
	const hkVector4& n = cp->m_separatingNormal;

	projectedVelocity = n.x * (velocityA.x - velocityB.x) + n.y * (velocityA.y - velocityB.y) + n.z * (velocityA.z - velocityB.z);
}

template <typename Body>
static bool IsSamePose(const Body& body, const hkpMotion* motion)
{
	return BitsEqual12(&body.centerOfMass, &motion->m_centerOfMass1) && BitsEqual16(&body.rotation, &motion->m_rotation1);
}

static void SetBodyPose(hkpMotion* motion, const hkVector4* transform, const hkVector4& centerOfMass, const hkVector4& rotation)
{
	memcpy(motion->m_transform, transform, sizeof(motion->m_transform));
	motion->m_centerOfMass1 = { centerOfMass.x, centerOfMass.y, centerOfMass.z, motion->m_centerOfMass1.w };
	motion->m_rotation1 = rotation;
}

static void ShowBody(SimulatedBody& body, hkpMotion* motion, float alpha)
{
	body.shownCenterOfMass = Lerp(body.prevCenterOfMass, body.centerOfMass, alpha);
	body.shownRotation = Nlerp(body.prevRotation, body.rotation, alpha);

	memcpy(body.shownTransform, body.transform, sizeof(body.shownTransform));
	BuildTransform(body.shownTransform, body.shownCenterOfMass, body.shownRotation, motion->m_centerOfMassLocal);

	SetBodyPose(motion, body.shownTransform, body.shownCenterOfMass, body.shownRotation);

	body.shown = true;
	g_shownBodyCount++;
}

static bool IsShowingBody(const SimulatedBody& body, const hkpMotion* motion)
{
	return body.shown && memcmp(motion->m_transform, body.shownTransform, sizeof(body.shownTransform)) == 0;
}

static void RestoreSimulatedBodies()
{
	if (g_shownBodyCount == 0) return;

	auto restore = [](uintptr_t entity)
	{
		auto it = g_simulatedBodies.find(entity);

		if (it != g_simulatedBodies.end() && it->second.shown)
		{
			SimulatedBody& body = it->second;
			hkpMotion* motion = GetMotion(entity);

			// Keep the pose the game gave the body since
			if (IsShowingBody(body, motion))
			{
				SetBodyPose(motion, body.transform, body.centerOfMass, body.rotation);
			}

			body.shown = false;
			g_shownBodyCount--;
		}

		return g_shownBodyCount != 0;
	};

	if (ForEachIslandEntityWhile(OFF_WORLD_ACTIVE_SIMULATION_ISLANDS, restore))
	{
		ForEachIslandEntityWhile(OFF_WORLD_INACTIVE_SIMULATION_ISLANDS, restore);
	}

	if (g_shownBodyCount != 0)
	{
		for (auto& [entity, body] : g_simulatedBodies)
		{
			body.shown = false;
		}

		g_shownBodyCount = 0;
	}
}

static void UpdateSimulatedBodies()
{
	float alpha = g_tickAccumulator / TARGET_FRAME_TIME;
	size_t seenBodies = 0;

	ForEachIslandEntity(OFF_WORLD_ACTIVE_SIMULATION_ISLANDS, [alpha, &seenBodies](uintptr_t entity)
	{
		hkpMotion* motion = GetMotion(entity);

		if (motion->m_type == MOTION_KEYFRAMED)
		{
			// The game moved it if it's elsewhere at the next step
			if (auto it = g_keyframedBodies.find(entity); it != g_keyframedBodies.end())
			{
				it->second.centerOfMass = motion->m_centerOfMass1;
				it->second.rotation = motion->m_rotation1;
			}

			return;
		}

		if (!IsDynamicMotion(motion->m_type)) return;

		SimulatedBody& body = g_simulatedBodies[entity];

		bool wasInTicks = body.tickedStep == g_stepCount && body.seenStep != 0;

		if (!wasInTicks || (!body.ticked && !IsSamePose(body, motion)))
		{
			body.interpolate = false;
		}

		body.centerOfMass = motion->m_centerOfMass1;
		body.rotation = motion->m_rotation1;

		if (body.seenStep != g_stepCount) seenBodies++;
		body.seenStep = g_stepCount;

		if (!body.interpolate)
		{
			body.prevCenterOfMass = body.centerOfMass;
			body.prevRotation = body.rotation;
			return;
		}

		// Nothing to interpolate for a resting body
		if (alpha < 1.0f && (!BitsEqual12(&body.prevCenterOfMass, &body.centerOfMass) || !BitsEqual16(&body.prevRotation, &body.rotation)))
		{
			// Only read while the body is shown
			memcpy(body.transform, motion->m_transform, sizeof(body.transform));
			ShowBody(body, motion, alpha);
		}
	});

	// Only when some body wasn't seen this step
	if (seenBodies != g_simulatedBodies.size())
	{
		std::erase_if(g_simulatedBodies, [](const auto& entry) { return entry.second.seenStep != g_stepCount; });
	}

	std::erase_if(g_keyframedBodies, [](const auto& entry) { return entry.second.seenStep != g_stepCount; });
}

static void ScaleCollisionQualityTimes(float scale)
{
	uintptr_t qualityInfos = *reinterpret_cast<uintptr_t*>(g_havokWorld + OFF_WORLD_COLLISION_DISPATCHER) + OFF_DISPATCHER_COLLISION_QUALITY_INFO;

	for (int i = 0; i < NUM_COLLISION_QUALITY_INFOS; i++)
	{
		for (size_t j = 0; j < std::size(OFF_COLLISION_QUALITY_TIMES); j++)
		{
			float* time = reinterpret_cast<float*>(qualityInfos + i * SIZE_COLLISION_QUALITY_INFO + OFF_COLLISION_QUALITY_TIMES[j]);
			g_savedCollisionQualityTimes[i][j] = *time;
			*time *= scale;
		}
	}
}

static void RestoreCollisionQualityTimes()
{
	uintptr_t qualityInfos = *reinterpret_cast<uintptr_t*>(g_havokWorld + OFF_WORLD_COLLISION_DISPATCHER) + OFF_DISPATCHER_COLLISION_QUALITY_INFO;

	for (int i = 0; i < NUM_COLLISION_QUALITY_INFOS; i++)
	{
		for (size_t j = 0; j < std::size(OFF_COLLISION_QUALITY_TIMES); j++)
		{
			*reinterpret_cast<float*>(qualityInfos + i * SIZE_COLLISION_QUALITY_INFO + OFF_COLLISION_QUALITY_TIMES[j]) = g_savedCollisionQualityTimes[i][j];
		}
	}
}

static void BlendControlState(uint8_t* out, const uint8_t* from, const uint8_t* to, float t)
{
	__m128 vt = _mm_set1_ps(t);

	for (size_t offset = 0; offset < sizeof(KinesisControl::tickState); offset += sizeof(hkVector4))
	{
		__m128 a = _mm_loadu_ps(reinterpret_cast<const float*>(from + offset));
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(to + offset));
		_mm_storeu_ps(reinterpret_cast<float*>(out + offset), LerpVector(a, b, vt));
	}
}

static void PrepareTick(const hkpSolverInfo* solverInfo, const hkStepInfo* stepInfo)
{
	if (g_isTickPrepared) return;
	g_isTickPrepared = true;

	g_tickStepInfo = { stepInfo->m_startTime, stepInfo->m_startTime + g_tickLength, g_tickLength, 1.0f / g_tickLength };

	// Same setup as the world does for its steps
	const hkVector4& gravity = *reinterpret_cast<hkVector4*>(g_havokWorld + OFF_WORLD_GRAVITY);
	float subStepLength = g_tickLength * solverInfo->m_invNumSteps;

	g_tickSolverInfo = *solverInfo;
	g_tickSolverInfo.m_deltaTime = subStepLength;
	g_tickSolverInfo.m_invDeltaTime = solverInfo->m_numSteps * g_tickStepInfo.m_invDeltaTime;
	g_tickSolverInfo.m_globalAccelerationPerSubStep = { gravity.x * subStepLength, gravity.y * subStepLength, gravity.z * subStepLength, gravity.w * subStepLength };
	g_tickSolverInfo.m_globalAccelerationPerStep = { gravity.x * g_tickLength, gravity.y * g_tickLength, gravity.z * g_tickLength, gravity.w * g_tickLength };

	if (!g_hasTickDeactivation)
	{
		g_hasTickDeactivation = true;
		g_tickDeactivationCounter = solverInfo->m_deactivationIntegrateCounter;
		g_tickDeactivationSelectFlags[0] = solverInfo->m_deactivationNumInactiveFramesSelectFlag[0];
		g_tickDeactivationSelectFlags[1] = solverInfo->m_deactivationNumInactiveFramesSelectFlag[1];
	}

	uint8_t counter = ++g_tickDeactivationCounter;

	if (((counter - 4) & 7) == 0) g_tickDeactivationSelectFlags[0] ^= 1;
	if ((counter & 7) == 0) g_tickDeactivationSelectFlags[0] ^= 2;

	if ((counter & 15) == 0)
	{
		g_tickDeactivationCounter = 0;
		g_tickDeactivationSelectFlags[1] = 1 - g_tickDeactivationSelectFlags[1];
	}

	g_tickSolverInfo.m_deactivationIntegrateCounter = g_tickDeactivationCounter;
	g_tickSolverInfo.m_deactivationNumInactiveFramesSelectFlag[0] = g_tickDeactivationSelectFlags[0];
	g_tickSolverInfo.m_deactivationNumInactiveFramesSelectFlag[1] = g_tickDeactivationSelectFlags[1];
}

static void IntegrateKeyframedBodies(const hkpSolverInfo* solverInfo, const hkStepInfo* stepInfo)
{
	if (g_keyframedEntities.empty()) return;

	int count = static_cast<int>(g_keyframedEntities.size());

	hkRigidMotionUtilApplyForcesAndStep.unsafe_ccall<int>(solverInfo, stepInfo, &solverInfo->m_globalAccelerationPerStep, g_keyframedEntities.data(), count, OFF_ENTITY_MOTION);
	hkpEntityAabbUtil_entityBatchRecalcAabb(*reinterpret_cast<uintptr_t*>(g_havokWorld + OFF_WORLD_COLLISION_INPUT), g_keyframedEntities.data(), count);
}

static IslandStep BeginIslandStep(const uintptr_t* entities, int numEntities, const hkpSolverInfo* solverInfo, const hkStepInfo* stepInfo)
{
	if (!g_isWorldStep) return IslandStep::Frame;

	g_keyframedEntities.clear();
	int dynamicCount = 0;

	for (int i = 0; i < numEntities; i++)
	{
		uint8_t type = GetMotion(entities[i])->m_type;

		if (IsDynamicMotion(type))
		{
			dynamicCount++;
		}
		else if (type == MOTION_KEYFRAMED)
		{
			g_keyframedEntities.push_back(entities[i]);
		}
		else
		{
			// Character motions, leave the island to the game
			return IslandStep::Frame;
		}
	}

	if (dynamicCount == 0) return IslandStep::Frame;

	for (uintptr_t entity : g_keyframedEntities)
	{
		hkpMotion* motion = GetMotion(entity);
		auto [it, isNew] = g_keyframedBodies.try_emplace(entity);
		KeyframedBody& body = it->second;

		if (body.seenStep != g_stepCount)
		{
			body.prevLinearVelocity = isNew ? hkVector4{} : body.linearVelocity;
			body.prevAngularVelocity = isNew ? hkVector4{} : body.angularVelocity;
			body.linearVelocity = motion->m_linearVelocity;
			body.angularVelocity = motion->m_angularVelocity;
			body.pathTime += stepInfo->m_deltaTime;
		}

		if (isNew || !IsSamePose(body, motion))
		{
			body.tickCenterOfMass = motion->m_centerOfMass1;
			body.tickRotation = motion->m_rotation1;
			body.pathCenterOfMass = motion->m_centerOfMass1;
			body.pathRotation = motion->m_rotation1;
			body.pathTime = stepInfo->m_deltaTime;
		}

		body.seenStep = g_stepCount;
		g_keyframedSeenStep = g_stepCount;
	}

	if (!g_isTickStep)
	{
		for (int i = 0; i < numEntities; i++)
		{
			hkpMotion* motion = GetMotion(entities[i]);
			if (!IsDynamicMotion(motion->m_type)) continue;

			// Same as hkSweptTransformUtil::freezeMotionState, the body rests at its simulated pose for this frame
			motion->m_centerOfMass0 = { motion->m_centerOfMass1.x, motion->m_centerOfMass1.y, motion->m_centerOfMass1.z, stepInfo->m_startTime };
			motion->m_centerOfMass1.w = 0.0f;
			motion->m_rotation0 = motion->m_rotation1;
			motion->m_deltaAngle = {};

			SimulatedBody& body = g_simulatedBodies[entities[i]];
			body.tickedStep = g_stepCount;
			body.ticked = false;
		}

		IntegrateKeyframedBodies(solverInfo, stepInfo);
		return IslandStep::Frozen;
	}

	PrepareTick(solverInfo, stepInfo);

	g_savedMotions.clear();

	for (int i = 0; i < numEntities; i++)
	{
		hkpMotion* motion = GetMotion(entities[i]);

		if (IsDynamicMotion(motion->m_type))
		{
			if (auto command = g_kinesisCommands.find(entities[i]); command != g_kinesisCommands.end())
			{
				const KinesisCommand& c = command->second;
				motion->m_linearVelocity = Add(motion->m_linearVelocity, Lerp(c.prevLinearVelocity, c.linearVelocity, g_kinesisBlend));
				motion->m_angularVelocity = Add(motion->m_angularVelocity, Lerp(c.prevAngularVelocity, c.angularVelocity, g_kinesisBlend));

				g_kinesisCommands.erase(command);
			}

			SimulatedBody& body = g_simulatedBodies[entities[i]];
			body.prevCenterOfMass = motion->m_centerOfMass1;
			body.prevRotation = motion->m_rotation1;
			body.tickedStep = g_stepCount;
			body.ticked = true;
			body.interpolate = true;
		}
		else
		{
			g_savedMotions.push_back({ entities[i], *motion });

			KeyframedBody& body = g_keyframedBodies[entities[i]];

			hkVector4 offset = Sub(motion->m_centerOfMass1, body.tickCenterOfMass);

			if (!(Length3(offset) <= KEYFRAMED_JUMP_DISTANCE))
			{
				body.tickCenterOfMass = motion->m_centerOfMass1;
				body.tickRotation = motion->m_rotation1;
				body.pathCenterOfMass = motion->m_centerOfMass1;
				body.pathRotation = motion->m_rotation1;
				body.pathTime = stepInfo->m_deltaTime;
				offset = {};
			}

			hkVector4 turn = GetRotationDelta(body.tickRotation, motion->m_rotation1);

			float pathTime = body.pathTime - stepInfo->m_deltaTime;
			hkVector4 pathVelocity = pathTime > 0.0f ? Scale(Sub(motion->m_centerOfMass1, body.pathCenterOfMass), 1.0f / pathTime) : hkVector4{};
			hkVector4 pathAngularVelocity = pathTime > 0.0f ? Scale(GetRotationDelta(body.pathRotation, motion->m_rotation1), 1.0f / pathTime) : hkVector4{};

			hkVector4 v = ClampLength(ClampLength(motion->m_linearVelocity, Length3(body.prevLinearVelocity)), Length3(pathVelocity));
			hkVector4 w = ClampLength(ClampLength(motion->m_angularVelocity, Length3(body.prevAngularVelocity)), Length3(pathAngularVelocity));

			body.pathCenterOfMass = motion->m_centerOfMass1;
			body.pathRotation = motion->m_rotation1;
			body.pathTime = stepInfo->m_deltaTime;

			float leadTime = g_tickStart + g_tickLength;
			hkVector4 lead = Scale(v, leadTime);
			hkVector4 leadTurn = Scale(w, leadTime);

			motion->m_linearVelocity = Scale(Add(offset, lead), 1.0f / g_tickLength);
			motion->m_angularVelocity = Scale(Add(turn, leadTurn), 1.0f / g_tickLength);

			BuildTransform(motion->m_transform, body.tickCenterOfMass, body.tickRotation, motion->m_centerOfMassLocal);
			motion->m_centerOfMass1 = { body.tickCenterOfMass.x, body.tickCenterOfMass.y, body.tickCenterOfMass.z, motion->m_centerOfMass1.w };
			motion->m_rotation1 = body.tickRotation;
		}
	}

	return IslandStep::Tick;
}

static void EndIslandTick(const uintptr_t* entities, int numEntities, const hkpSolverInfo* solverInfo, const hkStepInfo* stepInfo)
{
	for (const SavedMotion& saved : g_savedMotions)
	{
		hkpMotion* motion = GetMotion(saved.entity);
		KeyframedBody& body = g_keyframedBodies[saved.entity];

		body.tickCenterOfMass = motion->m_centerOfMass1;
		body.tickRotation = motion->m_rotation1;

		body.sweptMotion = *motion;
		body.sweptMotion.m_centerOfMass1.w = stepInfo->m_invDeltaTime;
		body.sweptStep = g_stepCount;

		*motion = saved.motion;
	}

	IntegrateKeyframedBodies(solverInfo, stepInfo);

	for (int i = 0; i < numEntities; i++)
	{
		hkpMotion* motion = GetMotion(entities[i]);

		if (IsDynamicMotion(motion->m_type))
		{
			motion->m_centerOfMass1.w = stepInfo->m_invDeltaTime;
		}
	}
}

static int __fastcall hkpWorld_stepDeltaTime_Hook(uintptr_t thisp, int, float physicsDeltaTime)
{
	g_havokWorld = thisp;

	RestoreSimulatedBodies();

	g_stepCount++;
	g_isTickPrepared = false;
	g_isTickQueryPrepared = false;
	g_isTickStep = false;
	g_frameDeltaTime = physicsDeltaTime;

	if (physicsDeltaTime > 0.0f)
	{
		float tickStart = TARGET_FRAME_TIME - g_tickAccumulator;
		g_tickAccumulator += physicsDeltaTime;

		if (g_tickAccumulator >= TARGET_FRAME_TIME)
		{
			g_tickLength = std::max(TARGET_FRAME_TIME, g_tickAccumulator - TARGET_FRAME_TIME);
			g_tickAccumulator -= g_tickLength;
			g_tickStart = tickStart;
			g_isTickStep = true;

			// Kinesis was evaluated at the start and the end of this frame, its command is taken where the tick starts in between
			g_kinesisBlend = std::clamp(tickStart / physicsDeltaTime, 0.0f, 1.0f);
		}
	}

	g_collisionTimeScale = g_isTickStep ? std::clamp(physicsDeltaTime / g_tickLength, 0.001f, 1.0f) : 1.0f;

	if (g_collisionTimeScale < 1.0f)
	{
		ScaleCollisionQualityTimes(g_collisionTimeScale);
	}

	g_isWorldStep = true;
	int result = hkpWorld_stepDeltaTime.unsafe_thiscall<int>(thisp, physicsDeltaTime);
	g_isWorldStep = false;

	if (g_collisionTimeScale < 1.0f)
	{
		RestoreCollisionQualityTimes();
	}

	UpdateSimulatedBodies();

	if (g_isTickStep)
	{
		if (g_kinesis.hasPendingState)
		{
			if (g_kinesis.hasPrevPendingState)
			{
				BlendControlState(g_kinesis.tickState, g_kinesis.prevPendingState, g_kinesis.pendingState, g_kinesisBlend);
			}
			else
			{
				memcpy(g_kinesis.tickState, g_kinesis.pendingState, sizeof(g_kinesis.tickState));
			}

			g_kinesis.hasPendingState = false;
			g_kinesis.hasPrevPendingState = false;
		}

		g_kinesisCommands.clear();
	}

	return result;
}

static int __cdecl hkpConstraintSolverSetup_solve_Hook(const hkStepInfo* stepInfo, const hkpSolverInfo* solverInfo, void* constraintQueryIn, uintptr_t island, const uintptr_t* bodies, int numBodies)
{
	switch (BeginIslandStep(bodies, numBodies, solverInfo, stepInfo))
	{
		case IslandStep::Frozen:
			return 0;

		case IslandStep::Tick:
		{
			if (!g_isTickQueryPrepared)
			{
				g_isTickQueryPrepared = true;
				memcpy(g_tickConstraintQuery, constraintQueryIn, sizeof(g_tickConstraintQuery));
				hkpConstraintQueryIn_set(g_tickConstraintQuery, &g_tickSolverInfo, &g_tickStepInfo);
			}

			int result = hkpConstraintSolverSetup_solve.unsafe_ccall<int>(&g_tickStepInfo, &g_tickSolverInfo, g_tickConstraintQuery, island, bodies, numBodies);
			EndIslandTick(bodies, numBodies, solverInfo, stepInfo);
			return result;
		}

		default:
			return hkpConstraintSolverSetup_solve.unsafe_ccall<int>(stepInfo, solverInfo, constraintQueryIn, island, bodies, numBodies);
	}
}

static int __cdecl hkRigidMotionUtilApplyForcesAndStep_Hook(const hkpSolverInfo* solverInfo, const hkStepInfo* info, const hkVector4* deltaVel, const uintptr_t* motions, int numMotions, int motionOffset)
{
	switch (BeginIslandStep(motions, numMotions, solverInfo, info))
	{
		case IslandStep::Frozen:
			return 0;

		case IslandStep::Tick:
		{
			int result = hkRigidMotionUtilApplyForcesAndStep.unsafe_ccall<int>(&g_tickSolverInfo, &g_tickStepInfo, &g_tickSolverInfo.m_globalAccelerationPerStep, motions, numMotions, motionOffset);
			EndIslandTick(motions, numMotions, solverInfo, info);
			return result;
		}

		default:
			return hkRigidMotionUtilApplyForcesAndStep.unsafe_ccall<int>(solverInfo, info, deltaVel, motions, numMotions, motionOffset);
	}
}

static void __cdecl hkpConstraintSolverSetup_oneStepIntegrate_Hook(const hkpSolverInfo* solverInfo, const hkStepInfo* stepInfo, const void* accumulators, const uintptr_t* entities, int numEntities)
{
	if (!g_isWorldStep || !g_isTickStep || !(g_frameDeltaTime > 0.0f))
	{
		hkpConstraintSolverSetup_oneStepIntegrate.unsafe_ccall<void>(solverInfo, stepInfo, accumulators, entities, numEntities);
		return;
	}

	g_toiTickedEntities.clear();
	g_toiFrameEntities.clear();

	for (int i = 0; i < numEntities; i++)
	{
		(IsTickedThisStep(entities[i]) ? g_toiTickedEntities : g_toiFrameEntities).push_back(entities[i]);
	}

	if (!g_toiTickedEntities.empty())
	{
		float scale = g_tickLength / g_frameDeltaTime;
		alignas(16) hkStepInfo tickStepInfo = { stepInfo->m_startTime, stepInfo->m_startTime + stepInfo->m_deltaTime * scale, stepInfo->m_deltaTime * scale, stepInfo->m_invDeltaTime / scale };

		hkpConstraintSolverSetup_oneStepIntegrate.unsafe_ccall<void>(solverInfo, &tickStepInfo, accumulators, g_toiTickedEntities.data(), static_cast<int>(g_toiTickedEntities.size()));

		for (uintptr_t entity : g_toiTickedEntities)
		{
			GetMotion(entity)->m_centerOfMass1.w = stepInfo->m_invDeltaTime;
		}
	}

	if (!g_toiFrameEntities.empty())
	{
		hkpConstraintSolverSetup_oneStepIntegrate.unsafe_ccall<void>(solverInfo, stepInfo, accumulators, g_toiFrameEntities.data(), static_cast<int>(g_toiFrameEntities.size()));
	}
}

static void __fastcall hkpContinuousSimulation_simulateToi_Hook(uintptr_t thisp, int, uintptr_t world, uintptr_t event, float physicsDeltaTime)
{
	const uintptr_t* entities = reinterpret_cast<uintptr_t*>(event + OFF_TOI_EVENT_ENTITIES);

	// Solve the collisions caught between the frames with the tick length too
	if (g_isWorldStep && (IsSimulatedInTicks(entities[0]) || IsSimulatedInTicks(entities[1])))
	{
		physicsDeltaTime = g_isTickStep ? g_tickLength : TARGET_FRAME_TIME;
	}

	hkpContinuousSimulation_simulateToi.unsafe_thiscall<void>(thisp, world, event, physicsDeltaTime);
}

static void __fastcall hkpContinuousSimulation_collideIslandNarrowPhaseContinuous_Hook(uintptr_t thisp, int, uintptr_t island, uintptr_t input)
{
	g_collideMotions.clear();

	const hkArray& entities = *reinterpret_cast<hkArray*>(island + OFF_ISLAND_ENTITIES);

	// Only a keyframed body in an island simulated in ticks this step has a sweep to swap
	for (int i = 0; g_keyframedSeenStep == g_stepCount && i < entities.m_size; i++)
	{
		hkpMotion* motion = GetMotion(entities.m_data[i]);
		if (motion->m_type != MOTION_KEYFRAMED) continue;

		auto it = g_keyframedBodies.find(entities.m_data[i]);
		if (it == g_keyframedBodies.end() || it->second.seenStep != g_stepCount) continue;

		KeyframedBody& body = it->second;
		g_collideMotions.push_back({ entities.m_data[i], *motion });

		if (body.sweptStep == g_stepCount)
		{
			*motion = body.sweptMotion;
			continue;
		}

		// Frozen with the bodies between the ticks
		BuildTransform(motion->m_transform, body.tickCenterOfMass, body.tickRotation, motion->m_centerOfMassLocal);
		motion->m_centerOfMass0 = { body.tickCenterOfMass.x, body.tickCenterOfMass.y, body.tickCenterOfMass.z, motion->m_centerOfMass0.w };
		motion->m_centerOfMass1 = { body.tickCenterOfMass.x, body.tickCenterOfMass.y, body.tickCenterOfMass.z, 0.0f };
		motion->m_rotation0 = body.tickRotation;
		motion->m_rotation1 = body.tickRotation;
		motion->m_deltaAngle = {};
	}

	g_isIslandCollide = true;
	hkpContinuousSimulation_collideIslandNarrowPhaseContinuous.unsafe_thiscall<void>(thisp, island, input);
	g_isIslandCollide = false;

	for (const SavedMotion& saved : g_collideMotions)
	{
		*GetMotion(saved.entity) = saved.motion;
	}
}

static void __cdecl hkpWorldCallbackUtil_fireContactPointAdded_Hook(uintptr_t world, uintptr_t event)
{
	int type = *reinterpret_cast<int*>(event + OFF_POINT_ADDED_TYPE);

	if (g_isWorldStep && type == POINT_ADDED_TYPE_TOI && g_frameDeltaTime > 0.0f && g_tickLength > 0.0f)
	{
		FixToiProjectedVelocity(event);
	}

	hkpWorldCallbackUtil_fireContactPointAdded.unsafe_ccall<void>(world, event);
}

static int __fastcall HavokManager_CloseHavok_Hook(uintptr_t thisp)
{
	// The world is destroyed
	g_havokWorld = 0;
	g_shownBodyCount = 0;
	g_simulatedBodies.clear();
	g_keyframedBodies.clear();
	g_kinesis = {};
	g_kinesisCommands.clear();

	return HavokManager_CloseHavok.unsafe_thiscall<int>(thisp);
}

static void CaptureKinesisVelocity(hkpMotion* motion)
{
	if (!g_kinesis.isCapturing || !IsDynamicMotion(motion->m_type)) return;

	uintptr_t entity = reinterpret_cast<uintptr_t>(motion) - OFF_ENTITY_MOTION;

	for (const CapturedVelocity& captured : g_kinesisCaptures)
	{
		if (captured.entity == entity) return;
	}

	g_kinesisCaptures.push_back({ entity, motion->m_linearVelocity, motion->m_angularVelocity });
}

static void __fastcall hkpMotion_setLinearVelocity_Hook(hkpMotion* thisp, int, const hkVector4* newVel)
{
	CaptureKinesisVelocity(thisp);
	hkpMotion_setLinearVelocity.unsafe_thiscall<void>(thisp, newVel);
}

static void __fastcall hkpMotion_setAngularVelocity_Hook(hkpMotion* thisp, int, const hkVector4* newVel)
{
	CaptureKinesisVelocity(thisp);
	hkpMotion_setAngularVelocity.unsafe_thiscall<void>(thisp, newVel);
}

static void __fastcall hkpMotion_applyLinearImpulse_Hook(hkpMotion* thisp, int, const hkVector4* imp)
{
	CaptureKinesisVelocity(thisp);
	hkpMotion_applyLinearImpulse.unsafe_thiscall<void>(thisp, imp);
}

static int __fastcall PlayerFireTKSM_ApplyKeyframeToTarget_Hook(uintptr_t thisp, int, int type, float frameTime)
{
	uintptr_t targetBody = *reinterpret_cast<uintptr_t*>(thisp + OFF_FIRE_TK_TARGET_BODY);

	if (targetBody == 0 || !IsDynamicMotion(GetMotion(targetBody)->m_type))
	{
		return PlayerFireTKSM_ApplyKeyframeToTarget.unsafe_thiscall<int>(thisp, type, frameTime);
	}

	// Asleep, or stepped every frame with a character in its island, the velocities have to apply right away
	if (!IsSimulatedInTicks(targetBody))
	{
		g_kinesis = {};
		g_kinesisCommands.erase(targetBody);

		return PlayerFireTKSM_ApplyKeyframeToTarget.unsafe_thiscall<int>(thisp, type, frameTime);
	}

	uint8_t* controlState = reinterpret_cast<uint8_t*>(thisp + OFF_FIRE_TK_ACCUMULATOR);

	bool isSameControl = thisp == g_kinesis.fireState && targetBody == g_kinesis.targetBody && memcmp(controlState, g_kinesis.pendingState, sizeof(g_kinesis.pendingState)) == 0;

	if (isSameControl)
	{
		memcpy(controlState, g_kinesis.tickState, sizeof(g_kinesis.tickState));
	}
	else
	{
		// A new hold, or the game reset the control
		g_kinesis.fireState = thisp;
		g_kinesis.targetBody = targetBody;
		g_kinesis.hasPendingState = false;
		g_kinesis.hasPrevPendingState = false;
		memcpy(g_kinesis.tickState, controlState, sizeof(g_kinesis.tickState));
	}

	// Evaluated after the world step, the held body would show its interpolated pose
	hkpMotion* motion = GetMotion(targetBody);
	auto shownBody = g_simulatedBodies.find(targetBody);
	bool isShowing = shownBody != g_simulatedBodies.end() && IsShowingBody(shownBody->second, motion);

	if (isShowing)
	{
		SimulatedBody& body = shownBody->second;
		SetBodyPose(motion, body.transform, body.centerOfMass, body.rotation);
	}

	g_kinesisCaptures.clear();
	g_kinesis.isCapturing = true;

	int result = PlayerFireTKSM_ApplyKeyframeToTarget.unsafe_thiscall<int>(thisp, type, std::max(frameTime, TARGET_FRAME_TIME));

	g_kinesis.isCapturing = false;

	if (isShowing)
	{
		SimulatedBody& body = shownBody->second;

		if (memcmp(motion->m_transform, body.transform, sizeof(body.transform)) == 0)
		{
			SetBodyPose(motion, body.shownTransform, body.shownCenterOfMass, body.shownRotation);
		}
		else
		{
			body.shown = false;
			g_shownBodyCount--;
		}
	}

	// Kept to blend with this one at the tick, a hold and a throw aren't blended
	bool canBlendState = isSameControl && g_kinesis.hasPendingState && g_kinesis.pendingType == type;

	if (canBlendState)
	{
		memcpy(g_kinesis.prevPendingState, g_kinesis.pendingState, sizeof(g_kinesis.prevPendingState));
	}

	g_kinesis.hasPrevPendingState = canBlendState;
	memcpy(g_kinesis.pendingState, controlState, sizeof(g_kinesis.pendingState));
	g_kinesis.hasPendingState = true;
	g_kinesis.pendingType = type;

	// The bodies keep their velocities until the tick, the ones the game steps itself take them now
	for (const CapturedVelocity& captured : g_kinesisCaptures)
	{
		if (!IsSimulatedInTicks(captured.entity)) continue;

		hkpMotion* capturedMotion = GetMotion(captured.entity);
		hkVector4 linearVelocity = Sub(capturedMotion->m_linearVelocity, captured.linearVelocity);
		hkVector4 angularVelocity = Sub(capturedMotion->m_angularVelocity, captured.angularVelocity);

		auto [it, isNew] = g_kinesisCommands.try_emplace(captured.entity);
		KinesisCommand& command = it->second;

		// The throw keeps its full speed instead of being blended with the hold before it
		bool canBlend = !isNew && command.type == type;
		command.prevLinearVelocity = canBlend ? command.linearVelocity : linearVelocity;
		command.prevAngularVelocity = canBlend ? command.angularVelocity : angularVelocity;
		command.linearVelocity = linearVelocity;
		command.angularVelocity = angularVelocity;
		command.type = type;

		capturedMotion->m_linearVelocity = captured.linearVelocity;
		capturedMotion->m_angularVelocity = captured.angularVelocity;
	}

	return result;
}

static void ApplyHavokPhysicsFix()
{
	if (!HavokPhysicsFix) return;

	DWORD addr_hkpWorld_stepDeltaTime = GetAddress(Addr::hkpWorld_stepDeltaTime);
	DWORD addr_hkpConstraintSolverSetup_solve = GetAddress(Addr::hkpConstraintSolverSetup_solve);
	DWORD addr_hkpConstraintSolverSetup_oneStepIntegrate = GetAddress(Addr::hkpConstraintSolverSetup_oneStepIntegrate);
	DWORD addr_hkRigidMotionUtilApplyForcesAndStep = GetAddress(Addr::hkRigidMotionUtilApplyForcesAndStep);
	DWORD addr_hkpContinuousSimulation_simulateToi = GetAddress(Addr::hkpContinuousSimulation_simulateToi);
	DWORD addr_hkpContinuousSimulation_collideIslandNarrowPhaseContinuous = GetAddress(Addr::hkpContinuousSimulation_collideIslandNarrowPhaseContinuous);
	DWORD addr_hkpWorldCallbackUtil_fireContactPointAdded = GetAddress(Addr::hkpWorldCallbackUtil_fireContactPointAdded);
	DWORD addr_hkpConstraintQueryIn_set = GetAddress(Addr::hkpConstraintQueryIn_set);
	DWORD addr_hkpEntityAabbUtil_entityBatchRecalcAabb = GetAddress(Addr::hkpEntityAabbUtil_entityBatchRecalcAabb);
	DWORD addr_HavokManager_CloseHavok = GetAddress(Addr::HavokManager_CloseHavok);
	DWORD addr_hkpMotion_setLinearVelocity = GetAddress(Addr::hkpMotion_setLinearVelocity);
	DWORD addr_hkpMotion_applyLinearImpulse = GetAddress(Addr::hkpMotion_applyLinearImpulse);
	DWORD addr_PlayerFireTKSM_ApplyKeyframeToTarget = GetAddress(Addr::PlayerFireTKSM_ApplyKeyframeToTarget);

	hkpConstraintQueryIn_set = reinterpret_cast<decltype(hkpConstraintQueryIn_set)>(addr_hkpConstraintQueryIn_set);
	hkpEntityAabbUtil_entityBatchRecalcAabb = reinterpret_cast<decltype(hkpEntityAabbUtil_entityBatchRecalcAabb)>(addr_hkpEntityAabbUtil_entityBatchRecalcAabb);

	hkpWorld_stepDeltaTime = HookHelper::CreateHook((void*)addr_hkpWorld_stepDeltaTime, &hkpWorld_stepDeltaTime_Hook);
	hkpConstraintSolverSetup_solve = HookHelper::CreateHook((void*)addr_hkpConstraintSolverSetup_solve, &hkpConstraintSolverSetup_solve_Hook);
	hkpConstraintSolverSetup_oneStepIntegrate = HookHelper::CreateHook((void*)addr_hkpConstraintSolverSetup_oneStepIntegrate, &hkpConstraintSolverSetup_oneStepIntegrate_Hook);
	hkRigidMotionUtilApplyForcesAndStep = HookHelper::CreateHook((void*)addr_hkRigidMotionUtilApplyForcesAndStep, &hkRigidMotionUtilApplyForcesAndStep_Hook);
	hkpContinuousSimulation_simulateToi = HookHelper::CreateHook((void*)addr_hkpContinuousSimulation_simulateToi, &hkpContinuousSimulation_simulateToi_Hook);
	hkpContinuousSimulation_collideIslandNarrowPhaseContinuous = HookHelper::CreateHook((void*)addr_hkpContinuousSimulation_collideIslandNarrowPhaseContinuous, &hkpContinuousSimulation_collideIslandNarrowPhaseContinuous_Hook);
	hkpWorldCallbackUtil_fireContactPointAdded = HookHelper::CreateHook((void*)addr_hkpWorldCallbackUtil_fireContactPointAdded, &hkpWorldCallbackUtil_fireContactPointAdded_Hook);
	HavokManager_CloseHavok = HookHelper::CreateHook((void*)addr_HavokManager_CloseHavok, &HavokManager_CloseHavok_Hook);
	PlayerFireTKSM_ApplyKeyframeToTarget = HookHelper::CreateHook((void*)addr_PlayerFireTKSM_ApplyKeyframeToTarget, &PlayerFireTKSM_ApplyKeyframeToTarget_Hook);
	hkpMotion_setLinearVelocity = HookHelper::CreateHook((void*)addr_hkpMotion_setLinearVelocity, &hkpMotion_setLinearVelocity_Hook);
	hkpMotion_setAngularVelocity = HookHelper::CreateHook((void*)(addr_hkpMotion_setLinearVelocity + 0x20), &hkpMotion_setAngularVelocity_Hook);
	hkpMotion_applyLinearImpulse = HookHelper::CreateHook((void*)addr_hkpMotion_applyLinearImpulse, &hkpMotion_applyLinearImpulse_Hook);
}