// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Cue.h"

namespace NSGameplayTags
{
	// Common
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Common_Dash, "GameplayCue.Common.Dash");
	
	// Damage
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Flash, "GameplayCue.Damage.Flash");
	
	// Hit Reaction
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Basic_Ranged, "GameplayCue.HitReaction.NormalHit.Basic.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Basic_Melee, "GameplayCue.HitReaction.NormalHit.Basic.Melee");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Barrier_Ranged, "GameplayCue.HitReaction.NormalHit.Barrier.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_NormalHit_Barrier_Melee, "GameplayCue.HitReaction.NormalHit.Barrier.Melee");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_CriticalHit_Basic_Ranged, "GameplayCue.HitReaction.CriticalHit.Basic.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_CriticalHit_Basic_Melee, "GameplayCue.HitReaction.CriticalHit.Basic.Melee");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_CriticalHit_Barrier_Ranged, "GameplayCue.HitReaction.CriticalHit.Barrier.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_HitReaction_CriticalHit_Barrier_Melee, "GameplayCue.HitReaction.CriticalHit.Barrier.Melee");
	
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
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Vanguard_BuffRangePulse, "GameplayCue.Vanguard.BuffRangePulse");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Vanguard_BarrierField_Deploy, "GameplayCue.Vanguard.BarrierField.Deploy");

	// Engineer
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_MuzzleFire, "GameplayCue.Engineer.ShotgunFire.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_Impact, "GameplayCue.Engineer.ShotgunFire.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_ShotgunFire_BulletTrail, "GameplayCue.Engineer.ShotgunFire.BulletTrail");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_Deactivate, "GameplayCue.Engineer.SpawnTurret.Deactivate");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_MuzzleFire, "GameplayCue.Engineer.SpawnTurret.MuzzleFire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_Impact, "GameplayCue.Engineer.SpawnTurret.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpawnTurret_BulletTrail, "GameplayCue.Engineer.SpawnTurret.BulletTrail");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_SpeedBuff, "GameplayCue.Engineer.SpeedBuff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Engineer_BuffRangePulse, "GameplayCue.Engineer.BuffRangePulse");
	
	//Enemy MotherShip
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning01, "GameplayCue.MotherShip.BombingRun.Warning1");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning02, "GameplayCue.MotherShip.BombingRun.Warning2");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning03, "GameplayCue.MotherShip.BombingRun.Warning3");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Warning04, "GameplayCue.MotherShip.BombingRun.Warning4");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Missile, "GameplayCue.MotherShip.BombingRun.Missile");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Explosion_Sound, "GameplayCue.MotherShip.Explosion.Sound");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Explosion_Effect, "GameplayCue.MotherShip.Explosion.Effect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_BombingRun_Decal, "GameplayCue.MotherShip.BombingRun.Decal");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_Cloaking, "GameplayCue.MotherShip.Cloaking");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_HomingMissile_Fire, "GameplayCue.MotherShip.HomingMissile.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_HomingMissile_Explosion, "GameplayCue.MotherShip.HomingMissile.Explosion");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_HomingMissile_Thrust, "GameplayCue.MotherShip.HomingMissile.Thrust");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_Phase1Barrier, "GameplayCue.MotherShip.Phase1Barrier");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_Phase2Shield, "GameplayCue.MotherShip.Phase2Shield");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_Tesla, "GameplayCue.MotherShip.Tesla");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_ControlDevice_TeslaCoil, "GameplayCue.MotherShip.ControlDevice.TeslaCoil");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_MotherShip_ControlDevice_PowerOff, "GameplayCue.MotherShip.ControlDevice.PowerOff");
	
	//EnemyDrone
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_EnemyDrone_Fire, "GameplayCue.EnemyDrone.Fire");
	
	// CompanionDrone
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Companion_BasicFire, "GameplayCue.Companion.BasicFire");
}
