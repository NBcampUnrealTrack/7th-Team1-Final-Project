// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Cue.h"

namespace NSGameplayTags
{
	// Common
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Common_Dash, "GameplayCue.Common.Dash");
	
	// Damage
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Flash, "GameplayCue.Damage.Flash");
	
	// Hit Reaction
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Basic, "GameplayCue.HitReaction.NormalHit.Basic");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Barrier, "GameplayCue.HitReaction.NormalHit.Barrier");
	
	// Ranger
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_AutoFire_MuzzleFire, "GameplayCue.Ranger.AutoFire.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_AutoFire_Impact, "GameplayCue.Ranger.AutoFire.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_AutoFire_BulletTrail, "GameplayCue.Ranger.AutoFire.BulletTrail");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_ProjectileShot_MuzzleFire, "GameplayCue.Ranger.ProjectileShot.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_ProjectileShot_Impact, "GameplayCue.Ranger.ProjectileShot.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_Grenade_Explosion, "GameplayCue.Ranger.Grenade.Explosion");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ranger_SpeedBuff, "GameplayCue.Ranger.SpeedBuff");

	// Vanguard
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Vanguard_BaseAttack_Flash, "GameplayCue.Vanguard.BaseAttack.Flash");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Vanguard_VanguardBuff, "GameplayCue.Vanguard.VanguardBuff");

	// Engineer
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_MuzzleFire, "GameplayCue.Engineer.ShotgunFire.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_Impact, "GameplayCue.Engineer.ShotgunFire.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_BulletTrail, "GameplayCue.Engineer.ShotgunFire.BulletTrail");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_Deactivate, "GameplayCue.Engineer.SpawnTurret.Deactivate");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_MuzzleFire, "GameplayCue.Engineer.SpawnTurret.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_Impact, "GameplayCue.Engineer.SpawnTurret.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_BulletTrail, "GameplayCue.Engineer.SpawnTurret.BulletTrail");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpeedBuff, "GameplayCue.Engineer.SpeedBuff");
	
	//Enemy MotherShip
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning01, "GameplayCue.MotherShip.BombingRun.Warning1");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning02, "GameplayCue.MotherShip.BombingRun.Warning2");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning03, "GameplayCue.MotherShip.BombingRun.Warning3");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning04, "GameplayCue.MotherShip.BombingRun.Warning4");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Missile, "GameplayCue.MotherShip_BombingRun_Missile");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Impact, "GameplayCue.MotherShip_BombingRun_Impact");
}
