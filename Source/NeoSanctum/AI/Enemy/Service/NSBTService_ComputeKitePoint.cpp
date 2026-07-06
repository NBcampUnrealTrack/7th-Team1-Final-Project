// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_ComputeKitePoint.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NeoSanctum/Character/Enemy/NSEnemyDrone.h"

UNSBTService_ComputeKitePoint::UNSBTService_ComputeKitePoint()
{
	NodeName = "Compute Kite Point";
	bNotifyTick = true;

	// 출력: MoveTarget(Vector) — 부모 BlackboardBase의 BlackboardKey
	BlackboardKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UNSBTService_ComputeKitePoint, BlackboardKey));

	// 입력: 타겟 액터(Object) — Vector가 아니라 ObjectFilter
	TargetActorKey.AddObjectFilter(
		this, GET_MEMBER_NAME_CHECKED(UNSBTService_ComputeKitePoint, TargetActorKey), AActor::StaticClass());
}

void UNSBTService_ComputeKitePoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

void UNSBTService_ComputeKitePoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn) return;

	const FVector PawnLoc = Pawn->GetActorLocation();

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		BB->SetValueAsVector(BlackboardKey.SelectedKeyName, PawnLoc);
		return;
	}

	const FVector TargetLoc = TargetActor->GetActorLocation();

	// 타겟→폰 방향 (XY 평면)
	FVector RadialDir = (PawnLoc - TargetLoc).GetSafeNormal2D();
	if (RadialDir.IsNearlyZero())
	{
		RadialDir = Pawn->GetActorForwardVector().GetSafeNormal2D();
		if (RadialDir.IsNearlyZero()) RadialDir = FVector::ForwardVector;
	}

	// 현재 방위에서 옆으로 회전 → 선회(회피)
	const FVector StrafedDir = FRotator(0.f, StrafeOffsetAngle * StrafeDirection, 0.f).RotateVector(RadialDir);

	// 링 위 목표점 (Z는 폰 고도 유지 — FlyMoveTo는 XY만 사용)
	FVector KitePoint = TargetLoc + StrafedDir * DesiredDistance;
	KitePoint.Z = PawnLoc.Z;

	BB->SetValueAsVector(BlackboardKey.SelectedKeyName, KitePoint);
}
