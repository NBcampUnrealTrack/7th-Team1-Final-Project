// Copyright 2026 One Team. All rights reserved.

#include "NSDamageRules.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GenericTeamAgentInterface.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

namespace
{
	// Source와 Target이 모두 팀 인터페이스가 있으면 같은 팀 여부를 판단
	// 팀 정보가 없으면 ASC/Health 검사로 넘겨서 일단 데미지를 받을 수 있게 함(예: 파괴가능한 오브젝트)
	bool HasSameTeam(const AActor* SourceActor, const AActor* TargetActor)
	{
		const IGenericTeamAgentInterface* SourceTeam = Cast<IGenericTeamAgentInterface>(SourceActor);
		const IGenericTeamAgentInterface* TargetTeam = Cast<IGenericTeamAgentInterface>(TargetActor);

		return SourceTeam &&
			TargetTeam &&
			SourceTeam->GetGenericTeamId() == TargetTeam->GetGenericTeamId();
	}

	// Health를 기준으로 생존 여부를 확인함
	// Health Attribute가 없으면 공격 가능한 대상이 아닌 것으로 보고 false를 반환
	bool HasHealth(const UAbilitySystemComponent* TargetASC)
	{
		for (const UAttributeSet* SpawnedAttributeSet : TargetASC->GetSpawnedAttributes())
		{
			if (!SpawnedAttributeSet)
			{
				continue;
			}

			FProperty* HealthProperty = SpawnedAttributeSet->GetClass()->FindPropertyByName(TEXT("Health"));
			if (!HealthProperty)
			{
				continue;
			}

			const FGameplayAttribute HealthAttribute(HealthProperty);
			if (TargetASC->HasAttributeSetForAttribute(HealthAttribute) &&
				TargetASC->GetNumericAttribute(HealthAttribute) > 0.0f)
			{
				return true;
			}
		}

		return false;
	}
}

bool NSDamageRules::CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor)
{
	// 피해 적용은 Target ASC가 실제로 있는 액터가 대상이 될 수 있음.
	const IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const UAbilitySystemComponent* TargetASC =
		TargetAbilityInterface ? TargetAbilityInterface->GetAbilitySystemComponent() : nullptr;

	if (!IsValid(SourceActor) ||
		!IsValid(TargetActor) ||
		SourceActor == TargetActor ||
		!IsValid(TargetASC))
	{
		return false;
	}

	// 팀킬 방지. 
	// Barrier와 Turret도 Player 팀이기 Player가 공격하는 경우 때문에 여기에서 제외됨
	if (HasSameTeam(SourceActor, TargetActor))
	{
		return false;
	}
	
	// 전부 통과하면 살아있는 대상인지 판단
	return IsAliveDamageableTarget(TargetASC);
}

bool NSDamageRules::IsAliveDamageableTarget(const UAbilitySystemComponent* TargetASC)
{
	// Dead 태그가 이미 있으면 Health 값과 무관하게 추가적인 데미지를 입힐 수 없도록 false
	// @민재 : 무적 태그 있는 몬스터 데미지 입힐 수 없도록 추가 State_Invincible
	if (!IsValid(TargetASC) ||
		TargetASC->HasMatchingGameplayTag(NSGameplayTags::State_Dead) ||
		TargetASC->HasMatchingGameplayTag(NSGameplayTags::State_Invincible))
	{
		return false;
	}
	
	// Health Attribute가 있는지 판단
	return HasHealth(TargetASC);
}
