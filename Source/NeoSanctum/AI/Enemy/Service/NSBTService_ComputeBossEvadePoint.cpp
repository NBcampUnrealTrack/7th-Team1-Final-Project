// Copyright 2026 One Team. All rights reserved.

#include "NSBTService_ComputeBossEvadePoint.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"
#include "NeoSanctum/Character/Enemy/NSBossMotherShip.h"

UNSBTService_ComputeBossEvadePoint::UNSBTService_ComputeBossEvadePoint()
{
	NodeName = "Compute Boss Evade Point";
	bNotifyTick = true;
	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;

	BlackboardKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UNSBTService_ComputeBossEvadePoint, BlackboardKey));
}

void UNSBTService_ComputeBossEvadePoint::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	RerollTimer = 0.f;
	bHasBaseAltitude = false;
}

void UNSBTService_ComputeBossEvadePoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ANSBossMotherShip* Boss = GetBossMotherShip(OwnerComp);
	if (!BB || !Boss) return;

	UNSFlyingLocomotionComponent* FlyingLocomotion = Boss->GetFlyingLocomotion();
	ANSBossArenaBounds* Arena = Boss->GetArenaBounds();
	if (!FlyingLocomotion || !Arena) return;

	if (!bHasBaseAltitude)
	{
		BaseAltitude = FlyingLocomotion->GetAltitude();
		bHasBaseAltitude = true;
	}

	UpdateRerollTimer(DeltaSeconds);

	const FVector BossLocation = Boss->GetActorLocation();
	FVector EvadeXY = BossLocation;
	float TargetAltitude = BaseAltitude;

	switch (CurrentAxis)
	{
	case EBossEvadeAxis::Left:
		EvadeXY = BossLocation - Boss->GetActorRightVector() * HorizontalEvadeDistance;
		break;
	case EBossEvadeAxis::Right:
		EvadeXY = BossLocation + Boss->GetActorRightVector() * HorizontalEvadeDistance;
		break;
	case EBossEvadeAxis::Up:
		TargetAltitude = BaseAltitude + VerticalEvadeDelta;
		break;
	case EBossEvadeAxis::Down:
		TargetAltitude = BaseAltitude - VerticalEvadeDelta;
		break;
	}

	const FVector ClampedXY = Arena->ClampPointToBounds(EvadeXY);

	FlyingLocomotion->SetAltitude(TargetAltitude);
	BB->SetValueAsVector(BlackboardKey.SelectedKeyName, ClampedXY);
}

void UNSBTService_ComputeBossEvadePoint::UpdateRerollTimer(float DeltaSeconds)
{
	RerollTimer -= DeltaSeconds;
	if (RerollTimer > 0.f) return;

	RerollTimer = RerollInterval;
	CurrentAxis = static_cast<EBossEvadeAxis>(FMath::RandRange(0, 3));
}

ANSBossMotherShip* UNSBTService_ComputeBossEvadePoint::GetBossMotherShip(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	return AIController ? Cast<ANSBossMotherShip>(AIController->GetPawn()) : nullptr;
}