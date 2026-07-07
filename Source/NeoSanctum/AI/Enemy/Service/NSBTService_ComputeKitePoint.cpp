// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_ComputeKitePoint.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyDrone.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"

UNSBTService_ComputeKitePoint::UNSBTService_ComputeKitePoint()
{
	NodeName = "Compute Kite Point";
	bNotifyTick = true;
	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;
	
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

void UNSBTService_ComputeKitePoint::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	
	CurrentStrafeDir = FMath::Sign(StrafeDirection) < 0 ? -1 : 1;
	NoisePhase = FMath::FRand() * 1000;
	RerollTimer = FMath::FRandRange(RerollIntervalMin, RerollIntervalMax);
	TimeAccum = 0.f;
	bWasRetreating = false;
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

	// 랜덤 상태 갱신 (노이즈 시간 누적 → 방향 재추첨 → 반응형 반전)
	TimeAccum += DeltaSeconds;
	UpdateStrafeDirection(DeltaSeconds);
	TryReactiveFlip(OwnerComp);

	// 노이즈가 반영된 최종 선회각/거리로 목표점 산출
	const float EffAngle    = ComputeEffectiveStrafeAngle();
	const float EffDistance = ComputeEffectiveDistance();

	const FVector StrafedDir = FRotator(0.f, EffAngle, 0.f).RotateVector(RadialDir);

	// 링 위 목표점 (Z는 폰 고도 유지 — FlyMoveTo는 XY만 사용)
	FVector KitePoint = TargetLoc + StrafedDir * EffDistance;
	KitePoint.Z = PawnLoc.Z;

	BB->SetValueAsVector(BlackboardKey.SelectedKeyName, KitePoint);
}

void UNSBTService_ComputeKitePoint::UpdateStrafeDirection(float DeltaSeconds)
{
	RerollTimer -= DeltaSeconds;
	if(RerollTimer <= 0)
	{
		if (FMath::FRand() < DirectionFlipChance)
		{
			CurrentStrafeDir *= -1.f;
		}
		RerollTimer = FMath::FRandRange(RerollIntervalMin, RerollIntervalMax);
	}
}

void UNSBTService_ComputeKitePoint::TryReactiveFlip(UBehaviorTreeComponent& OwnerComp)
{
	if (!bReactiveFlipOnRetreat)
	{
		return;
	}
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;
	
	ANSEnemyPawnBase* EnemyPawn = Cast<ANSEnemyPawnBase>(AIC->GetPawn());
	if (!EnemyPawn) return;

	UNSFlyingLocomotionComponent* FlyingLocomotionComponent = EnemyPawn->GetFlyingLocomotion();
	if (!FlyingLocomotionComponent) return;

	const bool bRetreating = FlyingLocomotionComponent->IsRetreating();

	// 후퇴 "진입" 엣지: 지난 프레임 false → 이번 프레임 true 인 순간에만 1회 반전
	if (!bWasRetreating && bRetreating)
	{
		CurrentStrafeDir *= -1.f;
		// 방금 뒤집은 방향이 Level 1 재추첨에 곧바로 덮이지 않도록 타이머를 새로 채운다
		RerollTimer = FMath::FRandRange(RerollIntervalMin, RerollIntervalMax);
	}

	bWasRetreating = bRetreating;   // 다음 프레임 비교용으로 현재 상태 저장
}

float UNSBTService_ComputeKitePoint::ComputeEffectiveStrafeAngle() const
{
	const float t = TimeAccum * NoiseFrequency + NoisePhase;
	const float AngleNoise = FMath::PerlinNoise1D(t) * AngleNoiseAmplitude;
	return (StrafeOffsetAngle + AngleNoise) * CurrentStrafeDir;
}

float UNSBTService_ComputeKitePoint::ComputeEffectiveDistance() const
{
	const float t = TimeAccum * NoiseFrequency + NoisePhase + 100.f;
	const float DistNoise = FMath::PerlinNoise1D(t) * DistanceNoiseAmplitude;
	return FMath::Max(0.f, DesiredDistance + DistNoise);
}


