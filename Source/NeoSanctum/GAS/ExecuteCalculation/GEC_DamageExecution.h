// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEC_DamageExecution.generated.h"

/**
 * BaseDamage와 Defense를 기반으로 최종 Damage 결정 
 */
UCLASS()
class NEOSANCTUM_API UGEC_DamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGEC_DamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput
	) const override;
};
