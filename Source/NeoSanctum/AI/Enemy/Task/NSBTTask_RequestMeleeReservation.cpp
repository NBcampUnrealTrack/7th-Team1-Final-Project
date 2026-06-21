// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_RequestMeleeReservation.h"

#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"

UNSBTTask_RequestMeleeReservation::UNSBTTask_RequestMeleeReservation()
{
	NodeName = TEXT("Request Melee Reservation");
}

EBTNodeResult::Type UNSBTTask_RequestMeleeReservation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANSEnemyAIController* Controller = Cast<ANSEnemyAIController>(OwnerComp.GetAIOwner());

	if (!Controller)
	{
		return EBTNodeResult::Failed;
	}

	return Controller->RequestMeleeAttackReservation()
		       ? EBTNodeResult::Succeeded
		       : EBTNodeResult::Failed;
}
