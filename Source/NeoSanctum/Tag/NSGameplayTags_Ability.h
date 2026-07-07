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
	
	// Vanguard Ability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Vanguard_BaseAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Vanguard_Flicker);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Vanguard_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Vanguard_VanguardBuff);
	
	// Engineer Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_AttachTurretSpawner);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Engineer_ThrowTurretSpawner);
	
	// Common Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Common_RequestReload);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Common_DamageTaken);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Common_Shield_Broken);

	// Vanguard Ability Event Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Vanguard_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Vanguard_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Vanguard_ComboWindowOpened);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Vanguard_DashAttackRecoverStarted);
	
	// Common Ability Activate Fail Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_OutOfAmmo);
}
