// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/GAS/GameplayAbility/GA_SkillBase.h"
#include "GA_Engineer_SpawnTurret.generated.h"

/**
 * Engineer turret placement ability.
 */
UCLASS()
class NEOSANCTUM_API UGA_Engineer_SpawnTurret : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_Engineer_SpawnTurret();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
