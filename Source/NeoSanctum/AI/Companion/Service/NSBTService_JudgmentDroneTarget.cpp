// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_JudgmentDroneTarget.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetSystemLibrary.h"

UNSBTService_JudgmentDroneTarget::UNSBTService_JudgmentDroneTarget()
{
	NodeName = "Judgment Drone Target";
	
	Interval = 0.15f;
	RandomDeviation = 0.02f;
	bNotifyTick = true;
	
	MoveTargetKey.AddVectorFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,MoveTargetKey)
		);
	TargetActorKey.AddObjectFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, TargetActorKey),
		AActor::StaticClass()
		);
}

void UNSBTService_JudgmentDroneTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		MoveTargetKey.ResolveSelectedKey(*BBAsset);
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

void UNSBTService_JudgmentDroneTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB) return;
	
	ANSBaseCompanionAI* DronePawn = Cast<ANSBaseCompanionAI>(AIController->GetPawn());
	ANSDroneAIController* DroneController = Cast<ANSDroneAIController>(AIController);
	if (!DroneController || !DronePawn) return;
	
	if (AActor* Currency = FindNearestActor(DronePawn, CurrencyClass, CurrencyDetectionRadius))
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, Currency);
		BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, Currency->GetActorLocation());
		return;
	}
	
	const FVector FollowPos = 
		DronePawn->GetOwnerPlayer()->GetActorLocation() + 
		DronePawn->GetOwnerPlayer()->GetActorRotation().RotateVector(FollowOffset);
	
	BB->ClearValue(TargetActorKey.SelectedKeyName);
	BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, FollowPos);
}


AActor* UNSBTService_JudgmentDroneTarget::FindNearestActor(AActor* InActor, TSubclassOf<AActor> FilterClass, float Radius) const
{
	if (!InActor || !FilterClass) return nullptr;
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(InActor);
	
	TArray<AActor*> FoundActors;
	UKismetSystemLibrary::SphereOverlapActors(
		InActor->GetWorld(),
		InActor->GetActorLocation(),
		Radius,
		ObjectTypes,
		FilterClass,
		IgnoreActors,
		FoundActors
		);
	
	AActor* NearestActor = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector Origin = InActor->GetActorLocation();
	
	for (AActor* CandiateActor : FoundActors)
	{
		if (!CandiateActor) continue;
		
		const float DistSq = FVector::DistSquared(Origin, CandiateActor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			NearestActor = CandiateActor;
		}
	}
	
	return NearestActor;
}
