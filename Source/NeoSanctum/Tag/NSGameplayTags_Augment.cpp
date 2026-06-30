// Copyright 2026 One Team. All rights reserved.

#include "NSGameplayTags_Augment.h"

namespace NSGameplayTags
{
	// 추첨 시 사용할 풀 식별자
	UE_DEFINE_GAMEPLAY_TAG(Augment_Pool_Normal,         "Augment.Pool.Normal");
	
	// 스택 GE의 Magnitude 전달용
	UE_DEFINE_GAMEPLAY_TAG(Augment_Pool_HighGrade,      "Augment.Pool.HighGrade");
	UE_DEFINE_GAMEPLAY_TAG(Augment_SetByCaller_Stack,   "Augment.SetByCaller.Stack");
	
	// 증강 카드와 증강 효과 정의 Row를 식별하는 태그.
	//공통
	//UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_,"Augment.Definition.Common.")
	
	//Common
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_MaxHealth, "Augment.Definition.Common.MaxHealth")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_BaseDamage, "Augment.Definition.Common.BaseDamage")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_MaxShield, "Augment.Definition.Common.MaxShield")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_ShieldRegen, "Augment.Definition.Common.ShieldRegen")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_CritChance, "Augment.Definition.Common.CritChance")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_Defence,"Augment.Definition.Common.Defence")
	
	//Rare
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DmgUpHpDown,"Augment.Definition.Common.DmgUpHpDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DmgUpDefDown,"Augment.Definition.Common.DmgUpDefDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DmgUpShdDown,"Augment.Definition.Common.DmgUpShdDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DmgUpCritChanDown,"Augment.Definition.Common.DmgUpCritChanDown")
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_CritChanUpHpDown,"Augment.Definition.Common.CritChanUpHpDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_CritChanUpShdDown,"Augment.Definition.Common.CritChanUpShdDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_CritChanUpDefDown,"Augment.Definition.Common.CritChanUpDefDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_CritChanUpDmgDown,"Augment.Definition.Common.CritChanUpDmgDown")
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_HpUpShdDown,"Augment.Definition.Common.HpUpShdDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_HpUpDefDown,"Augment.Definition.Common.HpUpDefDown")
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_ShdUpHpDown,"Augment.Definition.Common.ShdUpHpDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_ShdUpDefDown,"Augment.Definition.Common.ShdUpDefDown")
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_ShdRgnUpHpDown,"Augment.Definition.Common.ShdRgnUpHpDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_ShdRgnUpDefDown,"Augment.Definition.Common.ShdRgnUpDefDown")
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DefUpHpDown,"Augment.Definition.Common.DefUpHpDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DefUpShdDown,"Augment.Definition.Common.DefUpShdDown")
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Common_DefUpShdRgnDown,"Augment.Definition.Common.DefUpShdRgnDown")
	
	//Epic
	
	//레인저
	//UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_, "Augment.Definition.Ranger.");
	
	//Common
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_AutoFire_Damage, "Augment.Definition.Ranger.AutoFire.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_AutoFire_FireRate, "Augment.Definition.Ranger.AutoFire.FireRate");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_ProjectileShot_Damage, "Augment.Definition.Ranger.ProjectileShot.Damage");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_Grenade_Damage, "Augment.Definition.Ranger.Grenade.Damage");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_SpeedBuff_BuffPower, "Augment.Definition.Ranger.SpeedBuff.BuffPower");
	
	//Rare
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_ProjectileShot_SplashRadius, "Augment.Definition.Ranger.ProjectileShot.SplashRadius");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_Grenade_SplashRadius, "Augment.Definition.Ranger.Grenade.SplashRadius");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_SpeedBuff_Duration, "Augment.Definition.Ranger.SpeedBuff.Duration");
	
	//Epic
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_SpeedBuff_CoolDown, "Augment.Definition.Ranger.SpeedBuff.CoolDown");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_Grenade_CoolDown, "Augment.Definition.Ranger.Grenade.CoolDown");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_ProjectileShot_Cooldown, "Augment.Definition.Ranger.ProjectileShot.Cooldown");
	//Legendary
	
	//엔지니어
	//UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_, "Augment.Definition.Engineer.");
	//Common
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_Damage, "Augment.Definition.Engineer.ShotgunFire.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_FireRate, "Augment.Definition.Engineer.ShotgunFire.FireRate");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_Damage, "Augment.Definition.Engineer.SpawnTurret.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_FireRate, "Augment.Definition.Engineer.SpawnTurret.FireRate");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_Barrier_MaxHealth, "Augment.Definition.Engineer.Barrier.MaxHealth");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpeedBuff_BuffPower, "Augment.Definition.Engineer.SpeedBuff.BuffPower");
	
	//Rare
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_DetAtkRange, "Augment.Definition.Engineer.SpawnTurret.DetAtkRange");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_Accuracy, "Augment.Definition.Engineer.SpawnTurret.Accuracy");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_Barrier_Duration, "Augment.Definition.Engineer.Barrier.Duration");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_Barrier_Radius, "Augment.Definition.Engineer.Barrier.Radius");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpeedBuff_Duration, "Augment.Definition.Engineer.SpeedBuff.Duration");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpeedBuff_Radius, "Augment.Definition.Engineer.SpeedBuff.Radius");
	//Epic
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_Cooldown, "Augment.Definition.Engineer.SpawnTurret.Cooldown");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_Cooldown, "Augment.Definition.Engineer.Barrier.Cooldown");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpeedBuff_Cooldown, "Augment.Definition.Engineer.SpeedBuff.Cooldown");
	
	//Legendary
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_PelletSpread, "Augment.Definition.Engineer.ShotgunFire.PelletSpread");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_PelletCount, "Augment.Definition.Engineer.ShotgunFire.PelletCount");
}
