#pragma once

enum class GameBuild
{
	Unknown = 0,
	V1_0,
	Current
};

enum class Addr
{
	// HavokPhysicsFix
	hkpWorld_stepDeltaTime,
	hkpConstraintSolverSetup_solve,
	hkpConstraintSolverSetup_oneStepIntegrate,
	hkRigidMotionUtilApplyForcesAndStep,
	hkpContinuousSimulation_simulateToi,
	hkpContinuousSimulation_collideIslandNarrowPhaseContinuous,
	hkpWorldCallbackUtil_fireContactPointAdded,
	hkpConstraintQueryIn_set,
	hkpEntityAabbUtil_entityBatchRecalcAabb,
	HavokManager_CloseHavok,
	hkpMotion_setLinearVelocity,
	hkpMotion_applyLinearImpulse,
	PlayerFireTKSM_ApplyKeyframeToTarget,

	// HighCoreCPUFix
	CPUFix,

	// ThreadAffinityFix
	PresentationThreadAffinity,
	MainThreadAffinity,
	PresentationThreadCore,
	MainThreadCore,

	// VSyncRefreshRateFix
	SetHz,

	// FixFrameLimiter / VSyncRefreshRateFix / FixStreamingBudget
	fpsLimiter,
	FrameLimiterEnabledPtr,
	TargetFrameTimeMsPtr,

	// FixDifficultyRewards
	UIOptions_PersistableRestore,
	UIFrontendManagerPtr,
	OptionsDifficultyPtr,

	// FixSuitIDConflicts
	PickupItem_SpawnInit,

	// FixSaveStringHandling
	LoadSaveFileList,
	CopyStringFromSave,

	// FixAutomaticWeaponFireRate / FixMenuSpeed
	TimeManager_HandleEvents,
	Item_IsReadyToUse,
	Item_ResetUseTimer,
	FrameTimeSecPtr,
	SimTimeElapsedMSecPtr,

	// FixSolarArrayElevator
	EventScheduler_SendDelayedMsgToEventHandler,
	ElevatorFlushHook,

	// FixBlurResolution
	ScreenGaussianBlur_RenderImmediate,
	AlchemyZoomBlurShader_SetSizeAndCenterZoom,
	ScreenBloomBlur,
	ScreenDofBlur,
	ScreenGlowBlur,
	ScreenDistortBlur,

	// FixShadowBlur
	BlurAttenuationBuffer,

	// FixFlareArtifacts
	AddCoronaModulatedQuad,
	FlareSnapshot,
	FlareTextureSubst,
	DeviceCleanupPre,

	// FixVertexNormals
	ShaderTable,
	ScreenShaderTable,
	MotionBlurShaderTable,

	// FixClothPhysics
	VerletIntegrate,
	ClothComponent_ApplyWindForce,
	ClothComponent_Teleport,
	ClothRelaxation,
	CapeRelaxation,
	RagdollComponent_DriveRigidBodies,

	// FixMenuSpeed
	AptUpdate,
	UIMenuBase_MenuControlThread,
	PlayerTweakCameraModifier_Update,

	// FixGameClock
	GetCurTimeInMSec,

	// FixMainLoopSpin
	SoundProviderRWAC2_IsReadyForFrame,
	Time_GetCurTimeInMSec,
	SoundProviderPtr,

	// FixStreamingBudget
	TimeSlicer_GetTimeRemainingBeforeVBlank,
	PresentModePtr,
	PresentIntervalsPtr,

	// FixAudioSyncStall
	System_IsCommandComplete,
	Dac_GetSamplesToMix,
	DacThread_Sleep,

	// FixInputHistory
	ControllerManager_UpdateOneController,

	// FixImpalingProjectiles
	ImpalingProjectile_ProjectileTick,

	// FixExplosionDamage
	TEffectEFE_Constructor,
	TEffectEFE_Simulate,
	TEffectEFE_HitEntity,

	// FixOffscreenEffects
	TFXSequencer_Simulate,

	// AchievementSupport
	GetGameLanguage,
	AchievementImpl_HandleEvents,
	AchievementManager_UnlockAchievement,
	UpdateObtainedTrophy,
	AchievementImpl_PersistableRestore,

	// DisableOnlineFeatures
	UIComponentManager_ShowScreen_Nucleus_Connecting,
	StartNucleusLogin,
	ShopOfflineMessage,

	// IncreasedEntityPersistence
	EnemyLifetimeManager_ResetPopLimit,
	EnemyLifetimeManager_Ctor,

	// IncreasedDecalPersistence
	DecalVertexBuffer,
	CreateVertexBuffer1,
	CreateVertexBuffer2,
	VertexBufferSize,

	// SkipIntro
	UIScreenManager_ShowScreen,
	RtMoviePlayer_Play,

	// SkipArtificialLoadingDelay
	SkipArtificialLoadingDelay,

	// AutoResolution
	GetConfigInt,

	// FontScaling
	FontScaling,
	FontScaling2,

	// FOVScaling
	CameraManager_SetRenderCamera,
	CameraManager_GetActiveCamFov,
	PlayerFallSM_PushTrapCam,
	PlayerTransitionToGravitySM_PushLandingCam,
	Sentient_SpawnObserverPoleCamera,
	PairedAttackCoordinatorSM_PushCamera,
	PoleCamera_Init,

	// RawMouseInput / AutoHideMouseCursor
	ApplyControlConfiguration,
	UpdateMenuCursor,
	RE4ChaseCamera_Update,
	RE4ChaseCamera_UpdateState,
	OrbitCamera_Update,
	PlayerZGJumpSM_ProcessAimingControls,
	PlayerFPSAimSM_ProcessGroundAiming,
	PlayerDraggedSM_AdjustAim,
	PlayerStationaryShootingSM_AdjustAim,
	PlayerDecompressionReactComponent_AdjustCameraAndAim,
	PlayerHangingSM_UpdateAim,
	MouseGetDeviceState,
	SensitivityInterp,
	PlayerSpeedSettings_GetGunModifier,
	InputDeviceManagerPtr,
	HangingMinYawPtr,
	HangingMaxYawPtr,
	HangingMinPitchPtr,
	HangingMaxPitchPtr,
	HangingYawFactorPtr,
	ResponseCurvePtr,
	PlayerSpeedSettingsPtr,

	// FilterInputDevices
	IsXInputDevice,
	InitializeInputDevice,

	// DisableKeyboardHook
	SetWindowsHook,

	// ExtraMouseButtonBinding
	RemapVisitMapping,
	ApplyActionBinding,
	EvaluateKeyboardKeys,
	MouseDeviceUpdate,

	// AutoHideMouseCursor
	UpdateMenuCursorCall,

	// TextureFiltering
	TX_ChangeOptions_d3d,

	// DynamicShadowResolution
	ShadowRes,

	// ImprovedAntiAliasing / AchievementSupport
	VertexShaderPtr,
	PixelShaderPtr,
	VertexDeclarationPtr,
	StreamSourcesPtr,
	PixelShaderConstantsPtr,
	VertexShaderConstantsPtr,
	SamplerStatesPtr,
	RenderStatesPtr,
	PSTable,
	ScreenEdgeAA_Render,
	ScreenEdgeAA_RenderImmediate,
	FrameCopyValidPtr,
	AAValsFlagsPtr,

	// ImprovedAntiAliasing (Supersampling)
	ModeMatch,
	SetDisplayModeThunk,
	WindowResize,
	CursorUpdate,
	CursorRestore,
	CursorClip,
	DisplayModeGetters,
	MenuCursorDelta,
	CopyToBackBuffer,
	PresentParamsPtr,

	// ArchiveStreamHook
	UStreamer_DispatchChunk,

	// BonusUnlocks
	SaveManagerBootCheck,
	UnlockedContent_ClearUnlocked,
	IgnitionDoorSpawn,
	IgnitionDoorEntitlement,
	UnlockHandlerPtr,
	UnlockedContent_ForceUnlocked,
	PlayerStoreSM_AddStoreListItemsToStore,
	PlayerStore_AddItem,
	UnlockedContent_IsUnlocked,
	AchievementManager_IsAchievementCompleteByPlatformId,

	// MainLoopHook
	MainLoop,

	// ResolutionHook
	UpdateDisplaySettings,
	SetResolution,

	// D3D9ApiMidHook
	ResetSite1,
	ResetSite2,
	DevicePtr,
	Present1,
	Present2,

	Count
};

namespace Addresses
{
	inline GameBuild g_build = GameBuild::Unknown;
	inline uintptr_t g_moduleBase = 0;

	inline constexpr uintptr_t kAddressTable[static_cast<size_t>(Addr::Count)][2] =
	{
		// HavokPhysicsFix
		/* hkpWorld_stepDeltaTime                                     */ { 0x4681B0, 0x468850 },
		/* hkpConstraintSolverSetup_solve                             */ { 0x48F070, 0x48F710 },
		/* hkpConstraintSolverSetup_oneStepIntegrate                  */ { 0x48EB90, 0x48F230 },
		/* hkRigidMotionUtilApplyForcesAndStep                        */ { 0x492AB0, 0x493150 },
		/* hkpContinuousSimulation_simulateToi                        */ { 0x4DA670, 0x4DAD10 },
		/* hkpContinuousSimulation_collideIslandNarrowPhaseContinuous */ { 0x4D7970, 0x4D8010 },
		/* hkpWorldCallbackUtil_fireContactPointAdded                 */ { 0x47E4F0, 0x47EB90 },
		/* hkpConstraintQueryIn_set                                   */ { 0x474EB0, 0x475550 },
		/* hkpEntityAabbUtil_entityBatchRecalcAabb                    */ { 0x490760, 0x490E00 },
		/* HavokManager_CloseHavok                                    */ { 0x138BB90, 0x138C430 },
		/* hkpMotion_setLinearVelocity                                */ { 0x49B730, 0x49BDD0 },
		/* hkpMotion_applyLinearImpulse                               */ { 0x49B770, 0x49BE10 },
		/* PlayerFireTKSM_ApplyKeyframeToTarget                       */ { 0x8A3630, 0x8A3DE0 },

		// HighCoreCPUFix
		/* CPUFix                                                     */ { 0x833BD3, 0x834353 },

		// ThreadAffinityFix
		/* PresentationThreadAffinity                                 */ { 0x81C0B6, 0x81C756 },
		/* MainThreadAffinity                                         */ { 0x81C6C5, 0x81CD65 },
		/* PresentationThreadCore                                     */ { 0x205A264, 0x205B264 },
		/* MainThreadCore                                             */ { 0x205A260, 0x205B260 },

		// VSyncRefreshRateFix
		/* SetHz                                                      */ { 0x13C9080, 0x13C9920 },

		// FixFrameLimiter / VSyncRefreshRateFix / FixStreamingBudget
		/* fpsLimiter                                                 */ { 0x13CA960, 0x13CB200 },
		/* FrameLimiterEnabledPtr                                     */ { 0x211BDB5, 0x211CDD5 },
		/* TargetFrameTimeMsPtr                                       */ { 0x1B94CA4, 0x1B95CD4 },

		// FixDifficultyRewards
		/* UIOptions_PersistableRestore                               */ { 0xA44C10, 0xA453C0 },
		/* UIFrontendManagerPtr                                       */ { 0x201B0E4, 0x201C0E4 },
		/* OptionsDifficultyPtr                                       */ { 0x1F5F31C, 0x1F6031C },

		// FixSuitIDConflicts
		/* PickupItem_SpawnInit                                       */ { 0xDCE890, 0xDCEFE0 },

		// FixSaveStringHandling
		/* LoadSaveFileList                                           */ { 0x1402770, 0x1403010 },
		/* CopyStringFromSave                                         */ { 0x1400790, 0x1401030 },

		// FixAutomaticWeaponFireRate / FixMenuSpeed
		/* TimeManager_HandleEvents                                   */ { 0x76EF00, 0x76F5A0 },
		/* Item_IsReadyToUse                                          */ { 0xDA7B90, 0xDA82E0 },
		/* Item_ResetUseTimer                                         */ { 0xDA7C60, 0xDA83B0 },
		/* FrameTimeSecPtr                                            */ { 0x204C3FC, 0x204D3FC },
		/* SimTimeElapsedMSecPtr                                      */ { 0x204C3C0, 0x204D3C0 },

		// FixSolarArrayElevator
		/* EventScheduler_SendDelayedMsgToEventHandler                */ { 0x7A9840, 0x7A9EE0 },
		/* ElevatorFlushHook                                          */ { 0x7A9389, 0x7A9A29 },

		// FixBlurResolution
		/* ScreenGaussianBlur_RenderImmediate                         */ { 0x6E5CB0, 0x6E6350 },
		/* AlchemyZoomBlurShader_SetSizeAndCenterZoom                 */ { 0x6FCD00, 0x6FD3A0 },
		/* ScreenBloomBlur                                            */ { 0x6ECA69, 0x6ED109 },
		/* ScreenDofBlur                                              */ { 0x6E7731, 0x6E7DD1 },
		/* ScreenGlowBlur                                             */ { 0x6F139E, 0x6F1A3E },
		/* ScreenDistortBlur                                          */ { 0x6E3D3D, 0x6E43DD },

		// FixShadowBlur
		/* BlurAttenuationBuffer                                      */ { 0x796720, 0x796DC0 },

		// FixFlareArtifacts
		/* AddCoronaModulatedQuad                                     */ { 0x6A7620, 0x6A7CC0 },
		/* FlareSnapshot                                              */ { 0x6AAEFD, 0x6AB59D },
		/* FlareTextureSubst                                          */ { 0x13D7E54, 0x13D86F4 },
		/* DeviceCleanupPre                                           */ { 0x13C9280, 0x13C9B20 },

		// FixVertexNormals
		/* ShaderTable                                                */ { 0x1C59240, 0x1C5A240 },
		/* ScreenShaderTable                                          */ { 0x1EBA140, 0x1EBB140 },
		/* MotionBlurShaderTable                                      */ { 0x1F5A6BC, 0x1F5B6BC },

		// FixClothPhysics
		/* VerletIntegrate                                            */ { 0x10738D0, 0x1074020 },
		/* ClothComponent_ApplyWindForce                              */ { 0x1074A40, 0x1075190 },
		/* ClothComponent_Teleport                                    */ { 0x1074E30, 0x1075580 },
		/* ClothRelaxation                                            */ { 0x1073870, 0x1073FC0 },
		/* CapeRelaxation                                             */ { 0x1073B50, 0x10742A0 },
		/* RagdollComponent_DriveRigidBodies                          */ { 0xB94C05, 0xB95355 },

		// FixMenuSpeed
		/* AptUpdate                                                  */ { 0x70704A, 0x7076EA },
		/* UIMenuBase_MenuControlThread                               */ { 0x15EA270, 0x15EAB10 },
		/* PlayerTweakCameraModifier_Update                           */ { 0x1093CC0, 0x1094410 },

		// FixGameClock
		/* GetCurTimeInMSec                                           */ { 0x76EF72, 0x76F612 },

		// FixMainLoopSpin
		/* SoundProviderRWAC2_IsReadyForFrame                         */ { 0x7314B0, 0x731B50 },
		/* Time_GetCurTimeInMSec                                      */ { 0x81A140, 0x81A7E0 },
		/* SoundProviderPtr                                           */ { 0x20457E8, 0x20467E8 },

		// FixStreamingBudget
		/* TimeSlicer_GetTimeRemainingBeforeVBlank                    */ { 0x825DD0, 0x826470 },
		/* PresentModePtr                                             */ { 0x1F629FC, 0x1F639FC },
		/* PresentIntervalsPtr                                        */ { 0x211C178, 0x211D198 },

		// FixAudioSyncStall
		/* System_IsCommandComplete                                   */ { 0x536670, 0x536D10 },
		/* Dac_GetSamplesToMix                                        */ { 0x519FA0, 0x51A640 },
		/* DacThread_Sleep                                            */ { 0x52447F, 0x524B1F },

		// FixInputHistory
		/* ControllerManager_UpdateOneController                      */ { 0x77DB40, 0x77E1E0 },

		// FixImpalingProjectiles
		/* ImpalingProjectile_ProjectileTick                          */ { 0xDE0E60, 0xDE15B0 },

		// FixExplosionDamage
		/* TEffectEFE_Constructor                                     */ { 0x6BFED0, 0x6C0570 },
		/* TEffectEFE_Simulate                                        */ { 0x6C09C0, 0x6C1060 },
		/* TEffectEFE_HitEntity                                       */ { 0x6C04E0, 0x6C0B80 },

		// FixOffscreenEffects
		/* TFXSequencer_Simulate                                      */ { 0x6D8E70, 0x6D9510 },

		// AchievementSupport
		/* GetGameLanguage                                            */ { 0x4445D0, 0x444C50 },
		/* AchievementImpl_HandleEvents                               */ { 0xBA09D0, 0xBA1120 },
		/* AchievementManager_UnlockAchievement                       */ { 0x1398900, 0x13991A0 },
		/* UpdateObtainedTrophy                                       */ { 0x1398640, 0x1398EE0 },
		/* AchievementImpl_PersistableRestore                         */ { 0xB70B70, 0xB712C0 },

		// DisableOnlineFeatures
		/* UIComponentManager_ShowScreen_Nucleus_Connecting           */ { 0xA4EC00, 0xA4F3D0 },
		/* StartNucleusLogin                                          */ { 0xAA05F0, 0xAA0D40 },
		/* ShopOfflineMessage                                         */ { 0xEC4ACD, 0xEC521D },

		// IncreasedEntityPersistence
		/* EnemyLifetimeManager_ResetPopLimit                         */ { 0x10034A0, 0x1003BF0 },
		/* EnemyLifetimeManager_Ctor                                  */ { 0x1003230, 0x1003980 },

		// IncreasedDecalPersistence
		/* DecalVertexBuffer                                          */ { 0x6F0762, 0x6F0E02 },
		/* CreateVertexBuffer1                                        */ { 0x13D2CA9, 0x13D3549 },
		/* CreateVertexBuffer2                                        */ { 0x13D2D9E, 0x13D363E },
		/* VertexBufferSize                                           */ { 0x13D2471, 0x13D2D11 },

		// SkipIntro
		/* UIScreenManager_ShowScreen                                 */ { 0x71BE60, 0x71C500 },
		/* RtMoviePlayer_Play                                         */ { 0x13C50C0, 0x13C5960 },

		// SkipArtificialLoadingDelay
		/* SkipArtificialLoadingDelay                                 */ { 0xA412D9, 0xA41A89 },

		// AutoResolution
		/* GetConfigInt                                               */ { 0x40602D, 0x406037 },

		// FontScaling
		/* FontScaling                                                */ { 0xB8F9E3, 0xB90133 },
		/* FontScaling2                                               */ { 0xBB80AF, 0xBB87FF },

		// FOVScaling
		/* CameraManager_SetRenderCamera                              */ { 0x792DD0, 0x793470 },
		/* CameraManager_GetActiveCamFov                              */ { 0x792CE0, 0x793380 },
		/* PlayerFallSM_PushTrapCam                                   */ { 0x880B80, 0x881330 },
		/* PlayerTransitionToGravitySM_PushLandingCam                 */ { 0x8860B0, 0x886860 },
		/* Sentient_SpawnObserverPoleCamera                           */ { 0xEB94B0, 0xEB9C00 },
		/* PairedAttackCoordinatorSM_PushCamera                       */ { 0x10AA770, 0x10AAF30 },
		/* PoleCamera_Init                                            */ { 0xF6C1F0, 0xF6C940 },

		// RawMouseInput / AutoHideMouseCursor
		/* ApplyControlConfiguration                                  */ { 0x1096F20, 0x1097670 },
		/* UpdateMenuCursor                                           */ { 0xA5F820, 0xA5FF70 },
		/* RE4ChaseCamera_Update                                      */ { 0xF7A610, 0xF7AD60 },
		/* RE4ChaseCamera_UpdateState                                 */ { 0xF6E1CE, 0xF6E91E },
		/* OrbitCamera_Update                                         */ { 0xF79600, 0xF79D50 },
		/* PlayerZGJumpSM_ProcessAimingControls                       */ { 0x886F70, 0x887720 },
		/* PlayerFPSAimSM_ProcessGroundAiming                         */ { 0x898820, 0x898FD0 },
		/* PlayerDraggedSM_AdjustAim                                  */ { 0x86E3E0, 0x86EB80 },
		/* PlayerStationaryShootingSM_AdjustAim                       */ { 0x10ADE44, 0x10AE604 },
		/* PlayerDecompressionReactComponent_AdjustCameraAndAim       */ { 0xB9E31C, 0xB9EA6C },
		/* PlayerHangingSM_UpdateAim                                  */ { 0xE91604, 0xE91D54 },
		/* MouseGetDeviceState                                        */ { 0x140E659, 0x140EEF9 },
		/* SensitivityInterp                                          */ { 0x861B50, 0x8622D0 },
		/* PlayerSpeedSettings_GetGunModifier                         */ { 0x86C820, 0x86CFA0 },
		/* InputDeviceManagerPtr                                      */ { 0x201B0A8, 0x201C0A8 },
		/* HangingMinYawPtr                                           */ { 0x1F5F99C, 0x1F6099C },
		/* HangingMaxYawPtr                                           */ { 0x1F5FA18, 0x1F60A18 },
		/* HangingMinPitchPtr                                         */ { 0x1F5FA14, 0x1F60A14 },
		/* HangingMaxPitchPtr                                         */ { 0x1F5FA1C, 0x1F60A1C },
		/* HangingYawFactorPtr                                        */ { 0x1F5D0C0, 0x1F5E0C0 },
		/* ResponseCurvePtr                                           */ { 0x1F5D320, 0x1F5E320 },
		/* PlayerSpeedSettingsPtr                                     */ { 0x2086640, 0x2087640 },

		// FilterInputDevices
		/* IsXInputDevice                                             */ { 0x140D2F0, 0x140DB90 },
		/* InitializeInputDevice                                      */ { 0x77FB33, 0x7801D3 },

		// DisableKeyboardHook
		/* SetWindowsHook                                             */ { 0x449BF0, 0x44A270 },

		// ExtraMouseButtonBinding
		/* RemapVisitMapping                                          */ { 0x15F3640, 0x15F3EE0 },
		/* ApplyActionBinding                                         */ { 0x82E1E0, 0x82E960 },
		/* EvaluateKeyboardKeys                                       */ { 0x830770, 0x830EF0 },
		/* MouseDeviceUpdate                                          */ { 0x82D250, 0x82D9D0 },

		// AutoHideMouseCursor
		/* UpdateMenuCursorCall                                       */ { 0xA78039, 0xA78789 },

		// TextureFiltering
		/* TX_ChangeOptions_d3d                                       */ { 0x13D7880, 0x13D8120 },

		// DynamicShadowResolution
		/* ShadowRes                                                  */ { 0x7A5100, 0x7A57A0 },

		// ImprovedAntiAliasing / AchievementSupport
		/* VertexShaderPtr                                            */ { 0x21277A0, 0x21287C0 },
		/* PixelShaderPtr                                             */ { 0x21277A4, 0x21287C4 },
		/* VertexDeclarationPtr                                       */ { 0x21277A8, 0x21287C8 },
		/* StreamSourcesPtr                                           */ { 0x211E6A0, 0x211F6C0 },
		/* PixelShaderConstantsPtr                                    */ { 0x211D5A0, 0x211E5C0 },
		/* VertexShaderConstantsPtr                                   */ { 0x211E7A0, 0x211F7C0 },
		/* SamplerStatesPtr                                           */ { 0x2127BE0, 0x2128C00 },
		/* RenderStatesPtr                                            */ { 0x211D180, 0x211E1A0 },
		/* PSTable                                                    */ { 0x1EBA850, 0x1EBB850 },
		/* ScreenEdgeAA_Render                                        */ { 0x6E6F6A, 0x6E760A },
		/* ScreenEdgeAA_RenderImmediate                               */ { 0x6E6C00, 0x6E72A0 },
		/* FrameCopyValidPtr                                          */ { 0x2031E81, 0x2032E81 },
		/* AAValsFlagsPtr                                             */ { 0x20339C0, 0x20349C0 },

		// ImprovedAntiAliasing
		/* ModeMatch                                                  */ { 0x13CB1A0, 0x13CBA40 },
		/* SetDisplayModeThunk                                        */ { 0x13CCD80, 0x13CD620 },
		/* WindowResize                                               */ { 0x13CBA28, 0x13CC2C8 },
		/* CursorUpdate                                               */ { 0x44B9C6, 0x44C046 },
		/* CursorRestore                                              */ { 0x44BB17, 0x44C197 },
		/* CursorClip                                                 */ { 0x44BC79, 0x44C2F9 },
		/* DisplayModeGetters                                         */ { 0x13C9090, 0x13C9930 },
		/* MenuCursorDelta                                            */ { 0xA5FAD8, 0xA60228 },
		/* CopyToBackBuffer                                           */ { 0x13C9649, 0x13C9EE9 },
		/* PresentParamsPtr                                           */ { 0x211B734, 0x211C754 },

		// ArchiveStreamHook
		/* UStreamer_DispatchChunk                                    */ { 0x858950, 0x8590D0 },

		// BonusUnlocks
		/* SaveManagerBootCheck                                       */ { 0xA72A27, 0xA73177 },
		/* UnlockedContent_ClearUnlocked                              */ { 0x13539A0, 0x13541C0 },
		/* IgnitionDoorSpawn                                          */ { 0xDC5687, 0xDC5DD7 },
		/* IgnitionDoorEntitlement                                    */ { 0xDD9B4D, 0xDDA29D },
		/* UnlockHandlerPtr                                           */ { 0x20203B0, 0x20213B0 },
		/* UnlockedContent_ForceUnlocked                              */ { 0x1353960, 0x1354180 },
		/* PlayerStoreSM_AddStoreListItemsToStore                     */ { 0xE9F900, 0xEA0050 },
		/* PlayerStore_AddItem                                        */ { 0xBB51D0, 0xBB5920 },
		/* UnlockedContent_IsUnlocked                                 */ { 0x1353800, 0x1354020 },
		/* AchievementManager_IsAchievementCompleteByPlatformId       */ { 0x1398960, 0x1399200 },

		// MainLoopHook
		/* MainLoop                                                   */ { 0x44A220, 0x44A8A0 },

		// ResolutionHook
		/* UpdateDisplaySettings                                      */ { 0x13C9020, 0x13C98C0 },
		/* SetResolution                                              */ { 0x13CCC40, 0x13CD4E0 },

		// D3D9ApiMidHook
		/* ResetSite1                                                 */ { 0x13CB903, 0x13CC1A3 },
		/* ResetSite2                                                 */ { 0x13CB0D0, 0x13CB970 },
		/* DevicePtr                                                  */ { 0x211BDAC, 0x211CDCC },
		/* Present1                                                   */ { 0x13C96B5, 0x13C9F55 },
		/* Present2                                                   */ { 0x13CA4C0, 0x13CAD60 },
	};

	inline void SetBuild(GameBuild build, uintptr_t moduleBase)
	{
		g_build = build;
		g_moduleBase = moduleBase;
	}

	inline GameBuild GetBuild()
	{
		return g_build;
	}
}

inline uintptr_t GetAddress(Addr id)
{
	if (Addresses::g_build == GameBuild::Unknown)
		return 0;

	uintptr_t raw = Addresses::kAddressTable[static_cast<size_t>(id)][static_cast<int>(Addresses::g_build) - 1];
	if (raw == 0) return 0;

	return Addresses::g_moduleBase + (raw - 0x400000);
}