// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_DroneMoveTo.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"

UNSBTTask_DroneMoveTo::UNSBTTask_DroneMoveTo()
{
	NodeName = TEXT("Drone Move To");
	bNotifyTick = true;
	
	MoveTargetKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNSBTTask_DroneMoveTo, MoveTargetKey));
}

void UNSBTTask_DroneMoveTo::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		MoveTargetKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UNSBTTask_DroneMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ANSBaseDroneAI* DroneAI = AIController ? Cast<ANSBaseDroneAI>(AIController->GetPawn()) : nullptr;
	if (!DroneAI)
	{
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::InProgress;
}

void UNSBTTask_DroneMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	ANSBaseDroneAI* DroneAI = AIController ? Cast<ANSBaseDroneAI>(AIController->GetPawn()) : nullptr;
	if (!BBComp || !DroneAI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	const FVector MoveTarget = BBComp->GetValueAsVector(MoveTargetKey.SelectedKeyName);
	
	DroneAI->MoveTowards(MoveTarget);
	
	if (bFinishOnArrival)
	{
		const FVector Delta = MoveTarget - DroneAI->GetActorLocation();
		const float HorizontalDistSq = FVector(Delta.X, Delta.Y, 0.0f).SizeSquared();
		if (HorizontalDistSq <= FMath::Square(AcceptanceRadius))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}


