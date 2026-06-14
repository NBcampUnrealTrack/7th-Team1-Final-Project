// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 스킬 수치 조회에 사용하는 CombatStat 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ExplosionRadius);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ProjectileSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_MaxAmmo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_PelletCount);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_SpreadRange);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_FireRange);
}
