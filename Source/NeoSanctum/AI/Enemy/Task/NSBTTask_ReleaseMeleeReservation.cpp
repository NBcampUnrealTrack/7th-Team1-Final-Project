// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_ReleaseMeleeReservation.h"

#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"

UNSBTTask_ReleaseMeleeReservation::UNSBTTask_ReleaseMeleeReservation()
{
	NodeName = TEXT("Release Melee Reservation");
}

EBTNodeResult::Type UNSBTTask_ReleaseMeleeReservation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANSEnemyAIController* Controller = Cast<ANSEnemyAIController>(OwnerComp.GetAIOwner());

	if (!Controller)
	{
		return EBTNodeResult::Failed;
	}

	Controller->ReleaseMeleeAttackReservation(true);
	return EBTNodeResult::Succeeded;
}
