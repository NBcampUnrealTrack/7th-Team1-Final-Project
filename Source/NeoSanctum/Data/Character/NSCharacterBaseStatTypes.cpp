// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterBaseStatTypes.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

// StatTag → Row 필드 매핑, 파츠 비교 UI 등에서 캐릭터 초기값을 태그로 조회할 때 사용
float FNSCharacterBaseStatRow::GetValueForTag(const FGameplayTag& StatTag) const
{
	if (StatTag == NSGameplayTags::CombatStat_MaxHealth)             { return MaxHealth; }
	if (StatTag == NSGameplayTags::CombatStat_Damage)                { return BaseDamage; }
	if (StatTag == NSGameplayTags::CombatStat_Defense)               { return Defense; }
	if (StatTag == NSGameplayTags::CombatStat_MoveSpeed)             { return MoveSpeed; }
	if (StatTag == NSGameplayTags::CombatStat_CritChance)            { return CritChance; }
	if (StatTag == NSGameplayTags::CombatStat_CritDamage)            { return CritDamage; }
	if (StatTag == NSGameplayTags::CombatStat_MaxShield)             { return MaxShield; }
	if (StatTag == NSGameplayTags::CombatStat_ShieldRechargeRate)    { return ShieldRechargeRate; }
	if (StatTag == NSGameplayTags::CombatStat_ShieldRechargeCooldown){ return ShieldRechargeCooldown; }
	if (StatTag == NSGameplayTags::CombatStat_MaxDashCount)          { return MaxDashCount; }
	if (StatTag == NSGameplayTags::CombatStat_DashRegenRate)         { return DashRegenRate; }
	if (StatTag == NSGameplayTags::CombatStat_MaxAmmo)               { return MaxAmmo; }
	if (StatTag == NSGameplayTags::CombatStat_MaxSkill1Count)        { return MaxSkill1Count; }
	if (StatTag == NSGameplayTags::CombatStat_MaxSkill2Count)        { return MaxSkill2Count; }
	if (StatTag == NSGameplayTags::CombatStat_MaxSkill3Count)        { return MaxSkill3Count; }
	if (StatTag == NSGameplayTags::CombatStat_MaxJumpCount)          { return MaxJumpCount; }
	return 0.f;
}
