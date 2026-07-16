// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Ability.h"

namespace NSGameplayTags
{
	// Common Ability
	UE_DEFINE_GAMEPLAY_TAG(Ability_Common_Dash, "Ability.Common.Dash");	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Common_Parkour, "Ability.Common.Parkour");	
	
	// Ranger Ability
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_AutoFire, "Ability.Ranger.AutoFire");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_Reload, "Ability.Ranger.Reload");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_ProjectileShot, "Ability.Ranger.ProjectileShot");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_Grenade, "Ability.Ranger.Grenade");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_SpeedBuff, "Ability.Ranger.SpeedBuff");

	// Engineer Ability
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_ShotgunFire, "Ability.Engineer.ShotgunFire");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_Reload, "Ability.Engineer.Reload");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_SpawnTurret, "Ability.Engineer.SpawnTurret");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_SpeedBuff, "Ability.Engineer.SpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_Barrier, "Ability.Engineer.Barrier");
	
	// Vanguard Ability
	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_BaseAttack, "Ability.Vanguard.BaseAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_Flicker, "Ability.Vanguard.Flicker");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_ThrowBarrierField, "Ability.Vanguard.ThrowBarrierField");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_VanguardBuff, "Ability.Vanguard.VanguardBuff");
	
	// Engineer Ability Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Event_Engineer_AttachTurretSpawner, "Event.Engineer.AttachTurretSpawner");
	UE_DEFINE_GAMEPLAY_TAG(Event_Engineer_ThrowTurretSpawner, "Event.Engineer.ThrowTurretSpawner");
	
	// Common Ability Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Event_Common_RequestReload, "Event.Common.RequestReload");
	UE_DEFINE_GAMEPLAY_TAG(Event_Common_DamageTaken, "Event.Common.DamageTaken");
	UE_DEFINE_GAMEPLAY_TAG(Event_Common_Shield_Broken, "Event.Common.Shield.Broken");

	// Vanguard Ability Event Tags
	UE_DEFINE_GAMEPLAY_TAG(Event_Vanguard_Hit, "Event.Vanguard.Hit");
	UE_DEFINE_GAMEPLAY_TAG(Event_Vanguard_ComboWindowOpened, "Event.Vanguard.ComboWindowOpened");
	UE_DEFINE_GAMEPLAY_TAG(Event_Vanguard_DashAttackRecoverStarted, "Event.Vanguard.DashAttackRecoverStarted");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_Vanguard_AttachBarrierSpawner, "Event.Vanguard.AttachBarrierSpawner");
	UE_DEFINE_GAMEPLAY_TAG(Event_Vanguard_ThrowBarrierSpawner, "Event.Vanguard.ThrowBarrierSpawner");
	
	// Common Ability Activate Fail Tags
	UE_DEFINE_GAMEPLAY_TAG(Ability_ActivateFail_OutOfAmmo, "Ability.ActivateFail.OutOfAmmo");
}
