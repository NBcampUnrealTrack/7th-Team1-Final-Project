// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_CompanionActivateAbility.h"

UNSBTTask_CompanionActivateAbility::UNSBTTask_CompanionActivateAbility()
{
}

EBTNodeResult::Type UNSBTTask_CompanionActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                                                    uint8* NodeMemory)
{
	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

void UNSBTTask_CompanionActivateAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
}
