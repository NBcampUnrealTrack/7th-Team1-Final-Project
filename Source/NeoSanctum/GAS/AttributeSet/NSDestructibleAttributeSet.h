// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSDestructibleAttributeSet.generated.h"

/**
 *  Destructible Object를 위한 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSDestructibleAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 마지막으로 적용된 데미지의 EffectContext (서버에서 파괴 방향 계산용)
	FGameplayEffectContextHandle LastDamageContext;
};
