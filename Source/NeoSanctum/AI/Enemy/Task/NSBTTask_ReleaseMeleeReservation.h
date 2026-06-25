// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NSBTTask_ReleaseMeleeReservation.generated.h"

UCLASS()
class NEOSANCTUM_API UNSBTTask_ReleaseMeleeReservation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNSBTTask_ReleaseMeleeReservation();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
