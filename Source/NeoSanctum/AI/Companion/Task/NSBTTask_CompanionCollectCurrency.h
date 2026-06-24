// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NSBTTask_CompanionCollectCurrency.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTTask_CompanionCollectCurrency : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTTask_CompanionCollectCurrency();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	UPROPERTY(EditAnywhere, Category= "Currency")
	FBlackboardKeySelector TargetDropIdKey;
};
