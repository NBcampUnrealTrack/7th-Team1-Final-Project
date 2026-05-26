// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSMonsterAttributeSet.generated.h"

/**
 * 몬스터 전용 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSMonsterAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
