// Copyright 2026 One Team. All rights reserved.

#include "NSCombatStatAttributeMapping.h"
#include "GameplayEffect.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"

// StatTag를 공용 Attribute GE의 SetByCaller 태그로 변환하는 테이블.
// Count 계열처럼 Multiply를 허용하지 않는 Stat은 MultiplySetByCallerTag를 비워 둠.
const TArray<FNSCombatStatAttributeMapping>& NSCombatStatAttribute::GetMappings()
{
	static const TArray<FNSCombatStatAttributeMapping> Mappings =
	{
		{NSGameplayTags::CombatStat_Damage, NSGameplayTags::Effect_SetByCaller_BaseDamage_Add, NSGameplayTags::Effect_SetByCaller_BaseDamage_Multiply},
		{ NSGameplayTags::CombatStat_CritChance, NSGameplayTags::Effect_SetByCaller_CritChance_Add, NSGameplayTags::Effect_SetByCaller_CritChance_Multiply },
		{NSGameplayTags::CombatStat_CritDamage, NSGameplayTags::Effect_SetByCaller_CritDamage_Add, NSGameplayTags::Effect_SetByCaller_CritDamage_Multiply},
		{NSGameplayTags::CombatStat_MoveSpeed, NSGameplayTags::Effect_SetByCaller_MoveSpeed_Add, NSGameplayTags::Effect_SetByCaller_MoveSpeed_Multiply},
		{ NSGameplayTags::CombatStat_MaxHealth, NSGameplayTags::Effect_SetByCaller_MaxHealth_Add, NSGameplayTags::Effect_SetByCaller_MaxHealth_Multiply },
		{ NSGameplayTags::CombatStat_MaxShield, NSGameplayTags::Effect_SetByCaller_MaxShield_Add, NSGameplayTags::Effect_SetByCaller_MaxShield_Multiply },
		{ NSGameplayTags::CombatStat_Defense, NSGameplayTags::Effect_SetByCaller_Defense_Add, NSGameplayTags::Effect_SetByCaller_Defense_Multiply },
		{ NSGameplayTags::CombatStat_MaxAmmo, NSGameplayTags::Effect_SetByCaller_MaxAmmo_Add, NSGameplayTags::Effect_SetByCaller_MaxAmmo_Multiply },
		{ NSGameplayTags::CombatStat_ShieldRechargeRate, NSGameplayTags::Effect_SetByCaller_ShieldRechargeRate_Add, NSGameplayTags::Effect_SetByCaller_ShieldRechargeRate_Multiply },
		{ NSGameplayTags::CombatStat_ShieldRechargeCooldown, NSGameplayTags::Effect_SetByCaller_ShieldRechargeCooldown_Add, NSGameplayTags::Effect_SetByCaller_ShieldRechargeCooldown_Multiply },
		{ NSGameplayTags::CombatStat_MaxDashCount, NSGameplayTags::Effect_SetByCaller_MaxDashCount_Add, FGameplayTag() },
		{ NSGameplayTags::CombatStat_DashRegenRate, NSGameplayTags::Effect_SetByCaller_DashRegenRate_Add, NSGameplayTags::Effect_SetByCaller_DashRegenRate_Multiply },
		{ NSGameplayTags::CombatStat_MaxSkill1Count, NSGameplayTags::Effect_SetByCaller_MaxSkill1Count_Add, FGameplayTag() },
		{ NSGameplayTags::CombatStat_MaxSkill2Count, NSGameplayTags::Effect_SetByCaller_MaxSkill2Count_Add, FGameplayTag() },
		{ NSGameplayTags::CombatStat_MaxSkill3Count, NSGameplayTags::Effect_SetByCaller_MaxSkill3Count_Add, FGameplayTag() },
	};

	return Mappings;
}

const FNSCombatStatAttributeMapping* NSCombatStatAttribute::FindMapping(const FGameplayTag& StatTag)
{
	return GetMappings().FindByPredicate(
		[StatTag](const FNSCombatStatAttributeMapping& Mapping)
		{
			return Mapping.StatTag == StatTag;
		}
	);
}

void NSCombatStatAttribute::InitializeNeutralSetByCallers(const FGameplayEffectSpecHandle& SpecHandle)
{
	if (!SpecHandle.IsValid())
	{
		return;
	}

	for (const FNSCombatStatAttributeMapping& Mapping : GetMappings())
	{
		if (Mapping.AddSetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(Mapping.AddSetByCallerTag, 0.0f);
		}

		if (Mapping.MultiplySetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(Mapping.MultiplySetByCallerTag, 1.0f);
		}
	}
}
