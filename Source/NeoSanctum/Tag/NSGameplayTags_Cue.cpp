// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Cue.h"

namespace NSGameplayTags
{
	// Common
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Common_Dash, "GameplayCue.Common.Dash");
	
	// Damage
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Flash, "GameplayCue.Damage.Flash");
	
	// Ranger
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_AutoFire_MuzzleFire, "GameplayCue.Ranger.AutoFire.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_AutoFire_Impact, "GameplayCue.Ranger.AutoFire.Impact");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_ProjectileShot_MuzzleFire, "GameplayCue.Ranger.ProjectileShot.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_ProjectileShot_Impact, "GameplayCue.Ranger.ProjectileShot.Impact");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_SpeedBuff, "GameplayCue.Ranger.SpeedBuff");

	// Engineer
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_MuzzleFire, "GameplayCue.Engineer.ShotgunFire.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_Impact, "GameplayCue.Engineer.ShotgunFire.Impact");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpeedBuff, "GameplayCue.Engineer.SpeedBuff");
}
