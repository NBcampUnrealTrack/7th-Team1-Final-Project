// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace NSGameplayTags
{
	// 스킬 수치 조회에 사용하는 CombatStat 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Defense);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Duration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_BuffRadius);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ExplosionRadius);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ProjectileSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_ReloadSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_MaxAmmo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_PelletCount);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_PelletSpread);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_FireRange);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_DetectionRange);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_Accuracy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(CombatStat_MaxSpawnableAngle);
}
