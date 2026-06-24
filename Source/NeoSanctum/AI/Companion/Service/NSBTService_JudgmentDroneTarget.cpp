// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_JudgmentDroneTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"

UNSBTService_JudgmentDroneTarget::UNSBTService_JudgmentDroneTarget()
{
	NodeName = "Judgment Drone Target";
	
	Interval = 0.15f;
	RandomDeviation = 0.02f;
	bNotifyTick = true;
	
	MoveTargetKey.AddVectorFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,MoveTargetKey));
	/*CurrencyActorKey.AddObjectFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget, CurrencyActorKey),
		AActor::StaticClass());*/
	TargetDropIdKey.AddIntFilter(this,
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,TargetDropIdKey));
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
		/*CurrencyActorKey.ResolveSelectedKey(*BBAsset);*/
		TargetDropIdKey.ResolveSelectedKey(*BBAsset);
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
	
	const ECompanionState NewState = EvaluateState(CompanionPawn, BB);
	BB->SetValueAsEnum(StateKey.SelectedKeyName, static_cast<uint8>(NewState));
	CompanionPawn->SetCurrentState(NewState);
}


ECompanionState UNSBTService_JudgmentDroneTarget::EvaluateState(ANSBaseCompanionAI* CompanionPawn,
	UBlackboardComponent* BB) const
{
	AActor* CompanionOwner = CompanionPawn->GetOwnerPlayer();
	if (!CompanionOwner) return ECompanionState::Follow;
	
	const FVector FollowPos = 
		CompanionOwner->GetActorLocation() + CompanionOwner->GetActorRotation().RotateVector(FollowOffset);
	BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, FollowPos);
	
	if (AActor* Enemy = FindNearestActor(CompanionPawn, EnemyClass, CombatDetectionRadius, EnemyObjectTypes, true))
	{
		BB->SetValueAsObject(EnemyActorKey.SelectedKeyName, Enemy);
		CompanionPawn->SetCurrentEnemy(Enemy);
		return ECompanionState::Combat;
	}
	
	BB->ClearValue(EnemyActorKey.SelectedKeyName);
	CompanionPawn->SetCurrentEnemy(nullptr);
	
	// OwnerPawn 가져오기
	APawn* OwnerPawn = Cast<APawn>(CompanionOwner);
	if (!OwnerPawn) return ECompanionState::Follow;
	
	// OwnerPlayerState 가져오기
	ANSPlayerState* OwnerPS = OwnerPawn->GetPlayerState<ANSPlayerState>();
	if (!OwnerPS) return ECompanionState::Follow;
	
	// 재화쪽 SubSystem 가져오기
	if (UNSCurrencyDropSubsystem* DropSubsystem = CompanionPawn->GetWorld()->GetSubsystem<UNSCurrencyDropSubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("Getdropsubsystem"));
		const FVector FromLocation = CompanionPawn->GetActorLocation();
		int32 OutDropId = INDEX_NONE;
		FVector OutLocation = FVector::ZeroVector;
	
		if (DropSubsystem->FindNearestTrackableDrop(OwnerPS, FromLocation, CurrencyDetectionRadius, OutDropId,OutLocation))
		{
			BB->SetValueAsInt(TargetDropIdKey.SelectedKeyName, OutDropId);
			BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, OutLocation);
			return ECompanionState::Collect;
		}
	}
	
	/*if (AActor* Currency = FindNearestActor(CompanionPawn, CurrencyClass, CurrencyDetectionRadius, CurrencyObjectTypes))
	{
		BB->SetValueAsObject(CurrencyActorKey.SelectedKeyName, Currency);
		BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, Currency->GetActorLocation());
		return ECompanionState::Collect;
	}*/
	
	/*BB->ClearValue(CurrencyActorKey.SelectedKeyName);*/
	
	BB->SetValueAsInt(TargetDropIdKey.SelectedKeyName, INDEX_NONE);
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
