// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_ExecuteEnemyAbility.generated.h"

class UGameplayAbility;

UCLASS()
class NEOSANCTUM_API UNSBTTask_ExecuteEnemyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNSBTTask_ExecuteEnemyAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
private:
	void OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> CachedAttackAbilityClass;
};
