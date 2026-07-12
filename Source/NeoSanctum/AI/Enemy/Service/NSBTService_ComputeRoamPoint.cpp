// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_ComputeRoamPoint.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSBossArenaBounds.h"

UNSBTService_ComputeRoamPoint::UNSBTService_ComputeRoamPoint()
{
	NodeName = "Compute Roam Point";
	bNotifyTick = true;
	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;

	BlackboardKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UNSBTService_ComputeRoamPoint, BlackboardKey));
}

void UNSBTService_ComputeRoamPoint::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn) return;

	AnchorLocation = Pawn->GetActorLocation();
	bArenaBoundsSearched = false;
	CachedArenaBounds = nullptr;
	RerollTimer = RoamRerollInterval;

	RollNewRoamPoint(OwnerComp);
}

void UNSBTService_ComputeRoamPoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn) return;

	RerollTimer -= DeltaSeconds;

	const float Dist2D = FVector::Dist2D(Pawn->GetActorLocation(), CurrentRoamPoint);
	if (Dist2D <= ArrivalThreshold || RerollTimer <= 0.f)
	{
		RollNewRoamPoint(OwnerComp);
	}
}

void UNSBTService_ComputeRoamPoint::RollNewRoamPoint(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return;

	const float RandAngle = FMath::FRandRange(0.f, 360.f);
	const float RandRadius = FMath::FRandRange(0.f, RoamRadius);
	const FVector Offset = FRotator(0.f, RandAngle, 0.f).RotateVector(FVector(RandRadius, 0.f, 0.f));

	FVector NewPoint = AnchorLocation + Offset;
	NewPoint.Z = Pawn->GetActorLocation().Z;

	if (ANSBossArenaBounds* Arena = FindArenaBounds(OwnerComp))
	{
		NewPoint = Arena->ClampPointToBounds(NewPoint);
	}

	CurrentRoamPoint = NewPoint;
	RerollTimer = RoamRerollInterval;

	BB->SetValueAsVector(BlackboardKey.SelectedKeyName, CurrentRoamPoint);
}

ANSBossArenaBounds* UNSBTService_ComputeRoamPoint::FindArenaBounds(UBehaviorTreeComponent& OwnerComp)
{
	if (bArenaBoundsSearched) return CachedArenaBounds.Get();

	bArenaBoundsSearched = true;

	if (UWorld* World = OwnerComp.GetWorld())
	{
		for (TActorIterator<ANSBossArenaBounds> It(World); It; ++It)
		{
			CachedArenaBounds = *It;
			break;
		}
	}

	return CachedArenaBounds.Get();
}