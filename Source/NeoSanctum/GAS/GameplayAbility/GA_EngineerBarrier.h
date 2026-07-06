// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_BarrierBase.h"
#include "GA_EngineerBarrier.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UGA_EngineerBarrier : public UGA_BarrierBase
{
	GENERATED_BODY()
	
public:
	UGA_EngineerBarrier();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
