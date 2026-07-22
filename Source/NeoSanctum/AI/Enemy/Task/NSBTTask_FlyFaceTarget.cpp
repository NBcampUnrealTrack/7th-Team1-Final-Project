// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_FlyFaceTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UNSBTTask_FlyFaceTarget::UNSBTTask_FlyFaceTarget()
{
	NodeName = "Fly Face Target";
	bCreateNodeInstance = true;
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNSBTTask_FlyFaceTarget, TargetActorKey), AActor::StaticClass());
}

void UNSBTTask_FlyFaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UNSBTTask_FlyFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ElapsedTime = 0.f;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	return IsValid(TargetActor) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

void UNSBTTask_FlyFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!OwnerPawn || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!IsValid(TargetActor))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Forward2D = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget2D = (TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation()).GetSafeNormal2D();

	if (!Forward2D.IsNearlyZero() && !ToTarget2D.IsNearlyZero())
	{
		const float AngleError = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward2D, ToTarget2D), -1.f, 1.f)));
		if (AngleError <= AlignThresholdDegrees)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	ElapsedTime += DeltaSeconds;
	if (ElapsedTime >= MaxTurnInTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}