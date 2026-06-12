// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NSBTTask_CompanionActivateAbility.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_CompanionActivateAbility : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTTask_CompanionActivateAbility();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
protected:
	UPROPERTY(EditAnywhere, Category="Companion|Ability")
	FGameplayTagContainer ActivateAbility;
};
