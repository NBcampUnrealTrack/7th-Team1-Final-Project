// Copyright 2026 One Team. All rights reserved.


#include "NSBTService_JudgmentDroneTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UNSBTService_JudgmentDroneTarget::UNSBTService_JudgmentDroneTarget()
{
	NodeName = "Judgment Drone Target";
	
	Interval = 0.15f;
	RandomDeviation = 0.02f;
	bNotifyTick = true;
	
	MoveTargetKey.AddVectorFilter(
		this, 
		GET_MEMBER_NAME_CHECKED(UNSBTService_JudgmentDroneTarget,MoveTargetKey));
	TargetDropIdKey.AddIntFilter(
		this,
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
	
	ANSCompanionDroneAI* CompanionPawn = Cast<ANSCompanionDroneAI>(AIController->GetPawn());
	ANSDroneAIController* DroneController = Cast<ANSDroneAIController>(AIController);
	if (!DroneController || !CompanionPawn) return;
	
	const ECompanionState NewState = EvaluateState(CompanionPawn, BB);
	BB->SetValueAsEnum(StateKey.SelectedKeyName, static_cast<uint8>(NewState));
	CompanionPawn->SetCurrentState(NewState);
	
	// [추가] 사격은 상태와 무관하게 "적이 있으면" 상시 시도.
	// EvaluateState에서 이미 CurrentEnemy를 세팅/해제했으므로 그대로 재사용한다.
	// 실제 연사 간격은 GAS 쿨다운이 제어하므로, 서비스 틱(0.15초)마다 호출해도
	// 쿨다운 중이면 자동으로 no-op → 과발사 걱정 없음.
	if (CompanionPawn->GetCurrentEnemy())
	{
		TryActivateFire(CompanionPawn);
	}
}

ECompanionState UNSBTService_JudgmentDroneTarget::EvaluateState(ANSCompanionDroneAI* CompanionPawn,
	UBlackboardComponent* BB) const
{
	AActor* CompanionOwner = CompanionPawn->GetOwnerPlayer();
	if (!CompanionOwner) return ECompanionState::Follow;

	// 기본 이동 목표: 오너 옆 따라다니는 위치 (재화가 없을 때 사용)
	const FVector FollowPos =
		CompanionOwner->GetActorLocation() + CompanionOwner->GetActorRotation().RotateVector(FollowOffset);

	// ── 1) 재화 탐지 (이동 우선순위 최상위) ──────────────────────────────
	bool bHasDrop = false;
	FVector DropLocation = FVector::ZeroVector;

	if (APawn* OwnerPawn = Cast<APawn>(CompanionOwner))
	{
		ANSPlayerState* OwnerPS = OwnerPawn->GetPlayerState<ANSPlayerState>();
		UNSCurrencyDropSubsystem* DropSubsystem =
			CompanionPawn->GetWorld()->GetSubsystem<UNSCurrencyDropSubsystem>();
		if (OwnerPS && DropSubsystem)
		{
			int32 OutDropId = INDEX_NONE;
			FVector OutLocation = FVector::ZeroVector;
			if (DropSubsystem->FindNearestTrackableDrop(
				OwnerPS, CompanionPawn->GetActorLocation(), CurrencyDetectionRadius, OutDropId, OutLocation))
			{
				bHasDrop = true;
				DropLocation = OutLocation;
			}
		}
	}

	// ── 2) 적 탐지: 상태와 무관하게 "항상" 수행 ─────────────────────────
	// 여기서 SetCurrentEnemy를 호출하면 내부적으로 SetRotationTarget이 걸려
	// 이동 방향과 별개로 적을 계속 조준한다. 그리고 TickNode의 상시 사격이
	// 이 적을 향해 발사된다. → Collect(재화 이동) 중에도 사격이 유지되는 핵심.
	AActor* Enemy = FindNearestActor(CompanionPawn, EnemyClass, CombatDetectionRadius, EnemyObjectTypes, true);
	if (Enemy)
	{
		BB->SetValueAsObject(EnemyActorKey.SelectedKeyName, Enemy);
		CompanionPawn->SetCurrentEnemy(Enemy);
	}
	else
	{
		BB->ClearValue(EnemyActorKey.SelectedKeyName);
		CompanionPawn->SetCurrentEnemy(nullptr);
	}

	// ── 3) 이동 목표 & 상태 결정 (재화 > 전투 > 추종) ──────────────────
	if (bHasDrop)
	{
		// 재화가 있으면 적이 있어도 이동은 무조건 재화로. (수집 최우선)
		BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, DropLocation);
		return ECompanionState::Collect;
	}

	// 재화 없음: 오너 옆으로 이동. 적이 있으면 Combat(=조준/사격은 위에서 이미 처리),
	// 없으면 Follow. 두 상태 모두 이동 목표는 FollowPos로 동일하다.
	BB->SetValueAsVector(MoveTargetKey.SelectedKeyName, FollowPos);
	return Enemy ? ECompanionState::Combat : ECompanionState::Follow;
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
			
			if (EnemyASC->HasMatchingGameplayTag(NSGameplayTags::State_Enemy_MotherShip_Stealth)) continue;
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

void UNSBTService_JudgmentDroneTarget::TryActivateFire(const ANSCompanionDroneAI* Drone) const
{
	if (!Drone) return;
	
	UAbilitySystemComponent* ASC = Drone->GetAbilitySystemComponent();
	if (!ASC) return;
	
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(FireAbilityTag));
}
