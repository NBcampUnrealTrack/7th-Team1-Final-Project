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
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_ProjectileShot_SplashRadius, "Augment.Definition.Ranger.ProjectileShot.SplashRadius");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_ProjectileShot_Damage, "Augment.Definition.Ranger.ProjectileShot.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_AutoFire_Damage, "Augment.Definition.Ranger.AutoFire.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Ranger_AutoFire_FireRate, "Augment.Definition.Ranger.AutoFire.FireRate");
	
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_Damage, "Augment.Definition.Engineer.ShotgunFire.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_FireRate, "Augment.Definition.Engineer.ShotgunFire.FireRate");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_PelletCount, "Augment.Definition.Engineer.ShotgunFire.PelletCount");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_ShotgunFire_PelletSpread, "Augment.Definition.Engineer.ShotgunFire.PelletSpread");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_Damage, "Augment.Definition.Engineer.SpawnTurret.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_FireRate, "Augment.Definition.Engineer.SpawnTurret.FireRate");
	UE_DEFINE_GAMEPLAY_TAG(Augment_Definition_Engineer_SpawnTurret_Cooldown, "Augment.Definition.Engineer.SpawnTurret.Cooldown");
}
