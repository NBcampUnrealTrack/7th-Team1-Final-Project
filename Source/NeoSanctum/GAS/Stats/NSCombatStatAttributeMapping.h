// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

struct FGameplayEffectSpecHandle;

/**
 * CombatStat 태그를 공용 Attribute Init GE의 SetByCaller 태그 / 실제 Attribute로 변환하는 매핑
 * Attribute를 Add/Multiply로 건드리는 여러 시스템(Augment, 공통 업그레이드, 파츠 등)이 공유한다
 */
struct FNSCombatStatAttributeMapping
{
	FGameplayTag StatTag;
	FGameplayTag AddSetByCallerTag;
	FGameplayTag MultiplySetByCallerTag;

	// 이 StatTag가 가리키는 실제 Attribute, 라이브 스탯 조회(파츠 비교 UI 등)에 사용
	FGameplayAttribute Attribute;
};

namespace NSCombatStatAttribute
{
	const TArray<FNSCombatStatAttributeMapping>& GetMappings();

	const FNSCombatStatAttributeMapping* FindMapping(const FGameplayTag& StatTag);

	// 공용 GE에는 여러 Attribute Modifier가 항상 들어 있으므로, 적용 전 전체 SetByCaller를 중립값(Add=0, Multiply=1)으로 채움.
	void InitializeNeutralSetByCallers(const FGameplayEffectSpecHandle& SpecHandle);
}
