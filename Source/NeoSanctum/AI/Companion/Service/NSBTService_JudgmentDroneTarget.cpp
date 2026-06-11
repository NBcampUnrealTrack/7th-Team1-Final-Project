// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_JudgmentDroneTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"

UNSBTService_JudgmentDroneTarget::UNSBTService_JudgmentDroneTarget()
{
	NodeName = "Judgment Drone Target";
	
	Interval = 0.15f;
	RandomDeviation = 0.02f;
	bNotifyTick = true;
	
	MoveTargetKey.AddVectorFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,MoveTargetKey));
	TargetActorKey.AddObjectFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, TargetActorKey),
		AActor::StaticClass());
	EnemyTargetKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, EnemyTargetKey),
		AActor::StaticClass());
}

void UNSBTService_JudgmentDroneTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		MoveTargetKey.ResolveSelectedKey(*BBAsset);
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		EnemyTargetKey.ResolveSelectedKey(*BBAsset);
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
	
	if (AActor* Enemy = FindNearestActor(DronePawn, EnemyClass, CombatDetectionRadius,EnemyObjectTypes, true))
	{
		BB->SetValueAsObject(EnemyTargetKey.SelectedKeyName, Enemy);
		//BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, ComputeStandoffPosition(DronePawn, Enemy));
		TryActivateFire(DronePawn);
		return;
	}
	
	BB->ClearValue(EnemyTargetKey.SelectedKeyName);
	
	if (AActor* Currency = FindNearestActor(DronePawn, CurrencyClass, CurrencyDetectionRadius, CurrencyObjectTypes))
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, Currency);
		BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, Currency->GetActorLocation());
		return;
	}
	
	BB->ClearValue(TargetActorKey.SelectedKeyName);
	
	AActor* Owner = DronePawn->GetOwnerPlayer();
	if (!Owner) return;
	
	const FVector FollowPos = 
		Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(FollowOffset);
	BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, FollowPos);
}


AActor* UNSBTService_JudgmentDroneTarget::FindNearestActor(
	AActor* InActor, 
	TSubclassOf<AActor> FilterClass, 
	float Radius,
	const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
	bool bRequireAliveEnemy) const
{
	if (!InActor || !FilterClass) return nullptr;
	
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
		
		if (bRequireAliveEnemy)
		{
			UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandiateActor);
			if (!EnemyASC) continue;
		
			const float Health = EnemyASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
			if (Health <= 0.f) continue;
		}
		
		const float DistSq = FVector::DistSquared(Origin, CandiateActor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			NearestActor = CandiateActor;
		}
	}
	
	return NearestActor;
}

FVector UNSBTService_JudgmentDroneTarget::ComputeStandoffPosition(const AActor* Drone, const AActor* Enemy) const
{
	if (!Drone || !Enemy) return FVector::ZeroVector;
	
	FVector Dir = (Drone->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
	
	return Enemy->GetActorLocation() + (Dir * EnemyDistance);
}

void UNSBTService_JudgmentDroneTarget::TryActivateFire(const ANSBaseCompanionAI* Drone) const
{
	if (!Drone) return;
	
	UAbilitySystemComponent* ASC = Drone->GetAbilitySystemComponent();
	if (!ASC) return;
	
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(FireAbilityTag));
}
