// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 추첨 시 사용할 풀 식별자
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Pool_Normal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Pool_HighGrade);

	// 스택 GE의 Magnitude 전달용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_SetByCaller_Stack);
	
	// 증강 카드와 증강 효과 정의 Row를 식별하는 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Common_Toughness);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Ranger_AutoFire_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Ranger_AutoFire_FireRate);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Ranger_ProjectileShot_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Ranger_ProjectileShot_SplashRadius);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Ranger_ProjectileShot_Cooldown);
	
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_ShotgunFire_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_ShotgunFire_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_ShotgunFire_PelletCount);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_ShotgunFire_PelletSpread);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_SpawnTurret_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_SpawnTurret_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Augment_Definition_Engineer_SpawnTurret_Cooldown);
}
