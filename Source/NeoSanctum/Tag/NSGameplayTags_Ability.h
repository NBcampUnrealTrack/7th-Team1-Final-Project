// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// Common Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Common_Dash);
	
	// Ranger Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_AutoFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ranger_ProjectileShot);

	// Engineer Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_ShotgunFire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_SpawnTurret);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Engineer_SpeedBuff);
	
	// Engineer Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_AttachTurretSpawner);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_ThrowTurretSpawner);
}
