#pragma once

#include "Globals.cpp"

#include "Features/Fixes/HavokPhysicsFix.cpp"
#include "Features/Fixes/HighCoreCPUFix.cpp"
#include "Features/Fixes/VSyncRefreshRateFix.cpp"
#include "Features/Fixes/FixDifficultyRewards.cpp"
#include "Features/Fixes/FixSuitIDConflicts.cpp"
#include "Features/Fixes/FixSaveStringHandling.cpp"
#include "Features/Fixes/FixAutomaticWeaponFireRate.cpp"
#include "Features/Fixes/FixSolarArrayElevator.cpp"
#include "Features/Fixes/FixBlurResolution.cpp"
#include "Features/Fixes/FixShadowBlur.cpp"
#include "Features/Fixes/FixFlareArtifacts.cpp"
#include "Features/Fixes/FixGlassReflections.cpp"

#include "Features/General/AchievementSupport.cpp"
#include "Features/General/DisableOnlineFeatures.cpp"
#include "Features/General/IncreasedEntityPersistence.cpp"
#include "Features/General/IncreasedDecalPersistence.cpp"
#include "Features/General/SkipIntro.cpp"
#include "Features/General/SkipArtificialLoadingDelay.cpp"

#include "Features/Display/AutoResolution.cpp"
#include "Features/Display/FontScaling.cpp"

#include "Features/Input/RawMouseInput.cpp"
#include "Features/Input/FilterInputDevices.cpp"
#include "Features/Input/UseSDLControllerInput.cpp"

#include "Features/Graphics/TextureFiltering.cpp"
#include "Features/Graphics/DynamicShadowResolution.cpp"

#include "Features/DLC/ShopHooks.cpp"

#include "Features/Misc/MainLoopHook.cpp"
#include "Features/Misc/ResolutionHook.cpp"
#include "Features/Misc/D3D9ApiMidHook.cpp"

#include "Initialization/Initialization.cpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Prevents DLL from receiving thread notifications
			DisableThreadLibraryCalls(hModule);

			// Skip wrapper initialization when loaded as .asi
			if (!IsUALPresent())
			{
				SystemHelper::LoadProxyLibrary();
			}

			regOpenKeyHook = HookHelper::CreateHookAPI(L"advapi32.dll", "RegOpenKeyExW", &RegOpenKeyExW_Hook);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			if (UseSDLControllerInput)
			{
				ControllerHelper::ShutdownSDLGamepad();
			}
			break;
		}
	}
	return TRUE;
}
