// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_FlyMoveTo.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"

UNSBTTask_FlyMoveTo::UNSBTTask_FlyMoveTo()
{
	NodeName = "Enemy Fly Move To";
	bNotifyTick = true;
	MoveTargetKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNSBTTask_FlyMoveTo, MoveTargetKey));
}

void UNSBTTask_FlyMoveTo::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		MoveTargetKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UNSBTTask_FlyMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;
	
	ANSEnemyPawnBase* OwnerPawn = Cast<ANSEnemyPawnBase>(AIC->GetPawn());
	if (!OwnerPawn) return EBTNodeResult::Failed;
	
	if (!OwnerPawn->GetFlyingLocomotion()) return EBTNodeResult::Failed;
	
	return EBTNodeResult::InProgress;
}

void UNSBTTask_FlyMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	UBlackboardComponent* BB = Cast<UBlackboardComponent>(OwnerComp.GetBlackboardComponent());
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	} 
	
	AAIController* AIC = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (!AIC) 
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	ANSEnemyPawnBase* OwnerPawn = Cast<ANSEnemyPawnBase>(AIC->GetPawn());
	if (!OwnerPawn) 
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	UNSFlyingLocomotionComponent* FlyingLocomotion = Cast<UNSFlyingLocomotionComponent>(OwnerPawn->GetFlyingLocomotion());
	if (!FlyingLocomotion) 
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	const FVector MoveTarget = BB->GetValueAsVector(MoveTargetKey.SelectedKeyName);
	FlyingLocomotion->RequestMoveTowards(MoveTarget);
	
	if (bFinishOnArrival)
	{
		const float Dist2D = FVector::Dist2D(OwnerPawn->GetActorLocation(), MoveTarget);
		if (Dist2D <= AcceptanceRadius)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}
