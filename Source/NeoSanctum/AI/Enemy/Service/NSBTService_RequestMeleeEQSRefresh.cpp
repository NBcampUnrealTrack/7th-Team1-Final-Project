// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_RequestMeleeEQSRefresh.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UNSBTService_RequestMeleeEQSRefresh::UNSBTService_RequestMeleeEQSRefresh()
{
	NodeName = TEXT("Request Melee EQS Refresh");

	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;

	Interval = 0.25f;
	RandomDeviation = 0.05f;

	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(
			UNSBTService_RequestMeleeEQSRefresh,
			TargetActorKey),
		AActor::StaticClass());

	NeedsRefreshKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(
			UNSBTService_RequestMeleeEQSRefresh,
			NeedsRefreshKey));
}

void UNSBTService_RequestMeleeEQSRefresh::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BlackboardAsset);
		NeedsRefreshKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

void UNSBTService_RequestMeleeEQSRefresh::OnBecomeRelevant(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	QueryReferenceTargetLocation = FVector::ZeroVector;

	QueryReferenceTime = 0.0;
	bHasQueryReference = false;
}

void UNSBTService_RequestMeleeEQSRefresh::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UWorld* World = OwnerComp.GetWorld();

	if (!AIController ||
		!AIController->HasAuthority() ||
		!Blackboard ||
		!World)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!IsValid(TargetActor))
	{
		Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, false);

		bHasQueryReference = false;
		return;
	}

	const bool bNeedsRefresh = Blackboard->GetValueAsBool(NeedsRefreshKey.SelectedKeyName);

	if (bNeedsRefresh)
	{
		bHasQueryReference = false;
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	if (!bHasQueryReference)
	{
		QueryReferenceTargetLocation = TargetActor->GetActorLocation();

		QueryReferenceTime = CurrentTime;
		bHasQueryReference = true;
		return;
	}

	if (CurrentTime - QueryReferenceTime < MinimumRequeryInterval)
	{
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(
		QueryReferenceTargetLocation,
		TargetActor->GetActorLocation());

	if (DistanceSquared < FMath::Square(TargetRequeryDistance))
	{
		return;
	}

	Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, true);

	bHasQueryReference = false;
}
