// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Common Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Common_Dash);
	
	// Ranger Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_AutoFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_Reload);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_ProjectileShot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_Grenade);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_SpeedBuff);

	// Engineer Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_ShotgunFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_Reload);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_SpawnTurret);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_SpeedBuff);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_Barrier);
	
	// Engineer Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_AttachTurretSpawner);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_ThrowTurretSpawner);
	
	// Common Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Common_RequestReload);
}
