// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_BossSpawnDrone.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UGA_BossSpawnDrone : public UGA_EnemyAttackBase
{
	GENERATED_BODY()
	
public:
	UGA_BossSpawnDrone();
	
protected:
	virtual void HandleAttackEvent(const FGameplayEventData& Payload) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled) override;
};
