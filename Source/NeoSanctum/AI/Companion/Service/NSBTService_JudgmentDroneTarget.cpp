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
	CurrencyActorKey.AddObjectFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, CurrencyActorKey),
		AActor::StaticClass());
	EnemyActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, EnemyActorKey),
		AActor::StaticClass());
	StateKey.AddEnumFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,StateKey),
		StaticEnum<ECompanionState>());
}

void UNSBTService_JudgmentDroneTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		MoveTargetKey.ResolveSelectedKey(*BBAsset);
		CurrencyActorKey.ResolveSelectedKey(*BBAsset);
		EnemyActorKey.ResolveSelectedKey(*BBAsset);
		StateKey.ResolveSelectedKey(*BBAsset);
	}
}

void UNSBTService_JudgmentDroneTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB) return;
	
	ANSBaseCompanionAI* CompanionPawn = Cast<ANSBaseCompanionAI>(AIController->GetPawn());
	ANSDroneAIController* DroneController = Cast<ANSDroneAIController>(AIController);
	if (!DroneController || !CompanionPawn) return;
	
	BB->SetValueAsEnum(StateKey.SelectedKeyName, static_cast<uint8>(EvaluateState(CompanionPawn, BB)));
}


ECompanionState UNSBTService_JudgmentDroneTarget::EvaluateState(ANSBaseCompanionAI* CompanionPawn,
	UBlackboardComponent* BB) const
{
	AActor* Owner = CompanionPawn->GetOwnerPlayer();
	if (!Owner) return ECompanionState::Follow;
	
	const FVector FollowPos = 
		Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(FollowOffset);
	BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, FollowPos);
	
	if (AActor* Enemy = FindNearestActor(CompanionPawn, EnemyClass, CombatDetectionRadius, EnemyObjectTypes, true))
	{
		BB->SetValueAsObject(EnemyActorKey.SelectedKeyName, Enemy);
		return ECompanionState::Combat;
	}
	
	BB->ClearValue(EnemyActorKey.SelectedKeyName);
	
	if (AActor* Currency = FindNearestActor(CompanionPawn, CurrencyClass, CurrencyDetectionRadius, CurrencyObjectTypes))
	{
		BB->SetValueAsObject(CurrencyActorKey.SelectedKeyName, Currency);
		BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, Currency->GetActorLocation());
		return ECompanionState::Collect;
	}
	
	BB->ClearValue(CurrencyActorKey.SelectedKeyName);
	
	return ECompanionState::Follow;
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
