// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_RequestMeleeReservation.generated.h"

UCLASS()
class NEOSANCTUM_API UNSBTTask_RequestMeleeReservation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNSBTTask_RequestMeleeReservation();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
