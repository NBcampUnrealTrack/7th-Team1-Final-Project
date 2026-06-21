// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Common
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Common_Dash);
	
	// Damage
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage_Flash);
	
	// Ranger
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_AutoFire_MuzzleFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_AutoFire_Impact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_AutoFire_BulletTrail);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_ProjectileShot_MuzzleFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_ProjectileShot_Impact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_Grenade_Explosion);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ranger_SpeedBuff);
	
	// Engineer
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Engineer_ShotgunFire_MuzzleFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Engineer_ShotgunFire_Impact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Engineer_ShotgunFire_BulletTrail);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Engineer_SpawnTurret_Deactivate)
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Engineer_SpeedBuff);
}
