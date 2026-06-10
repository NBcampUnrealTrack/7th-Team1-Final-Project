// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 스킬들의 수치를 캐싱하는 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ExplosionRadius);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ProjectileSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_MaxAmmo);
}
