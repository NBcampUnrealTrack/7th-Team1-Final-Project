// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDroneAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyDrone.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"


// Sets default values
ANSEnemyDroneAIController::ANSEnemyDroneAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComponent");
	
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
	SightConfig->SightRadius = 6000.f;
	SightConfig->LoseSightRadius = 6500.f;
	SightConfig->PeripheralVisionAngleDegrees = 180.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;   // 플레이어(적 팀) 감지
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(*SightConfig->GetSenseImplementation());
}

#pragma region Lifecycle

void ANSEnemyDroneAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	const UNSEnemyData* EnemyData = GetControlledEnemyData();
	if (!EnemyData) return;
	
	StartEnemyBrain(EnemyData);
	InitBBState();
	ResetTargetingState();
	
	AIPerceptionComponent->
	OnTargetPerceptionUpdated.AddDynamic(this, &ANSEnemyDroneAIController::OnTargetPerceptionUpdated);
}

void ANSEnemyDroneAIController::OnUnPossess()
{
	StopEnemyBrain("Drone UnPossess");
	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->
		OnTargetPerceptionUpdated.RemoveDynamic(this, &ANSEnemyDroneAIController::OnTargetPerceptionUpdated);
	}
	ResetTargetingState();
	CachedBBComp = nullptr;
	Super::OnUnPossess();
}

void ANSEnemyDroneAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateEnemyPhase();

	ANSEnemyDrone* DronePawn = Cast<ANSEnemyDrone>(GetPawn());

	if (IsDroneAIBlocked())
	{
		SyncFlyingRotationTarget(nullptr);
		if (DronePawn) DronePawn->SetManeuvering(false);
		return;
	}

	if (UNSEnemyThreatComponent* DroneThreatComponent = GetEnemyThreatComponent())
	{
		if (DroneThreatComponent->CanEvaluateTarget())
		{
			UpdateTargetSelection();
			DroneThreatComponent->ScheduleNextEvaluation();
		}
	}

	UpdateCurrentTargetBlackboard();
	CanUseAnyAttackByDistance();

	const bool bCanAttack = CachedBBComp && CachedBBComp->GetValueAsBool(CanAttackKey);
	const bool bIsAttacking = CachedBBComp && CachedBBComp->GetValueAsBool(IsAttackingKey);
	const bool bFaceTarget = bCanAttack || bIsAttacking;

	SyncFlyingRotationTarget(bFaceTarget ? GetCurrentTargetActor() : nullptr);

	if (DronePawn) DronePawn->SetManeuvering(!bFaceTarget);

}

#pragma endregion 

#pragma region Targeting

void ANSEnemyDroneAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor && GetTeamAttitudeTo(*Actor) == ETeamAttitude::Hostile)
	{
		UNSEnemyThreatComponent* EnemyDroneThreatComponent = GetEnemyThreatComponent();
		if (!EnemyDroneThreatComponent) return;
		
		if (!IsValidLivingTarget(Actor))
		{
			EnemyDroneThreatComponent->RemoveTarget(Actor, true);
			return;
		}
		
		EnemyDroneThreatComponent->UpdateThreatFromStimulus(Actor, Stimulus);
		UpdateTargetSelection();
	}
}

void ANSEnemyDroneAIController::UpdateTargetSelection()
{
	UNSEnemyThreatComponent* EnemyDroneThreatComponent = GetEnemyThreatComponent();
	if (!EnemyDroneThreatComponent) return;
	
	AActor* CurrentTarget = EnemyDroneThreatComponent->GetCurrentTarget();
	
	const bool bIsAttacking = CachedBBComp && CachedBBComp->GetValueAsBool(IsAttackingKey);
	const bool bCanMaintainCurrentTarget = CurrentTarget && IsValidLivingTarget(CurrentTarget);
	
	const FNSEnemyThreatUpdateResult Result =
		EnemyDroneThreatComponent->UpdateTarget(bIsAttacking, bCanMaintainCurrentTarget);
	
	if (Result.bTargetChanged)
	{
		ClearAttackState();
	}

	UpdateCurrentTargetBlackboard();
}

void ANSEnemyDroneAIController::UpdateCurrentTargetBlackboard()
{
	if (!CachedBBComp) return;

	UNSEnemyThreatComponent* EnemyDroneThreatComponent = GetEnemyThreatComponent();
	if (!EnemyDroneThreatComponent) return;

	AActor* CurrentTarget = EnemyDroneThreatComponent->GetCurrentTarget();
	if (!IsValidLivingTarget(CurrentTarget))
	{
		ClearTargetBB(false);
		return;
	}

	CachedBBComp->SetValueAsObject(TargetActorKey, CurrentTarget);

	FVector TargetLocation = CurrentTarget->GetActorLocation();
	EnemyDroneThreatComponent->TryGetLastKnownLocation(CurrentTarget, TargetLocation);
	CachedBBComp->SetValueAsVector(TargetLastKnownLocationKey, TargetLocation);
}

AActor* ANSEnemyDroneAIController::GetCurrentTargetActor() const
{
	UNSEnemyThreatComponent* EnemyDroneThreatComponent = GetEnemyThreatComponent();
	return  EnemyDroneThreatComponent ? EnemyDroneThreatComponent->GetCurrentTarget() : nullptr;
}

void ANSEnemyDroneAIController::ResetTargetingState()
{
	if (UNSEnemyThreatComponent* ThreatComp = GetEnemyThreatComponent())
	{
		ThreatComp->ResetThreatState();
	}
	ClearTargetBB(true);
}

#pragma endregion

#pragma region SelectAttack

const FNSEnemyAttackRow* ANSEnemyDroneAIController::GetAttackRowByDistance()
{
	if (IsDroneAIBlocked())
	{
		SetCanAttackBB(false);
		return nullptr;
	}
	
	const FNSEnemyAttackRow* CurrentAttackRow = FindAttackRowByDistance(true);
	SetCanAttackBB(CurrentAttackRow != nullptr);
	return CurrentAttackRow;
}

bool ANSEnemyDroneAIController::CanUseAnyAttackByDistance()
{
	if (IsDroneAIBlocked())
	{
		SetCanAttackBB(false);
		return false;
	}
	
	const FNSEnemyAttackRow* CurrentAttackRow = FindAttackRowByDistance(false);
	SetCanAttackBB(CurrentAttackRow != nullptr);
	return CurrentAttackRow != nullptr;
}

const FNSEnemyAttackRow* ANSEnemyDroneAIController::FindAttackRowByDistance(bool bSelectWeightedAttack)
{
	APawn* OwnerPawn = GetPawn();
	AActor* CurrentTarget = GetCurrentTargetActor();
	if (!OwnerPawn || !IsValidLivingTarget(CurrentTarget))
	{
		ClearAttackState();
		return nullptr;
	}
	
	UNSEnemyAttackComponent* DroneAttackComponent = GetEnemyAttackComponent();
	UNSEnemyTargetComponent* DroneTargetComponent = GetEnemyTargetComponent();
	
	if (!DroneAttackComponent || !DroneTargetComponent)
	{
		ClearAttackState();
		return nullptr;
	}
	
	bool bHasLineOfSight = false;
	AActor* AttackActor = DroneTargetComponent->ResolveAttackActor(CurrentTarget, bHasLineOfSight);
	if (!AttackActor)
	{
		ClearAttackState();
		return nullptr;
	}
	const float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	
	const FNSEnemyAttackRow* SelectedAttack =
		DroneAttackComponent->SelectAttack(CurrentTarget, AttackActor, Distance, bHasLineOfSight, bSelectWeightedAttack);
	if (!SelectedAttack)
	{
		ClearAttackState();
		return nullptr;
	}

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, bHasLineOfSight);
	}
	
	return SelectedAttack;
}

void ANSEnemyDroneAIController::RecordAttackUsed(const FNSEnemyAttackRow& AttackRow)
{
	UNSEnemyThreatComponent* DroneThreatComponent = GetEnemyThreatComponent();
	if (DroneThreatComponent)
	{
		DroneThreatComponent->NotifyAttackStarted();
	}
	
	UNSEnemyAttackComponent* DroneAttackComponent = GetEnemyAttackComponent();
	if (DroneAttackComponent)
	{
		DroneAttackComponent->RecordAttackUsed(AttackRow);
	}
	SetIsAttackingBB(true);
}

#pragma endregion

void ANSEnemyDroneAIController::InitBBState()
{
	if (!CachedBBComp) return;

	ClearAttackState();
	ClearTargetBB(false);

	CachedBBComp->SetValueAsBool(PhasePatternLockedKey, false);
	CachedBBComp->SetValueAsName(CurrentPhaseIdKey, NAME_None);  
}

void ANSEnemyDroneAIController::SetCanAttackBB(bool bCanAttack)
{
	if (CachedBBComp) CachedBBComp->SetValueAsBool(CanAttackKey, bCanAttack);
}

void ANSEnemyDroneAIController::SetIsAttackingBB(bool bIsAttacking)
{
	if (CachedBBComp) CachedBBComp->SetValueAsBool(IsAttackingKey, bIsAttacking);
}

void ANSEnemyDroneAIController::ClearAttackState()
{
	SetCanAttackBB(false);
	SetIsAttackingBB(false);
}

void ANSEnemyDroneAIController::ClearTargetBB(bool bClearCanAttack)
{
	if (!CachedBBComp) return;

	CachedBBComp->ClearValue(TargetActorKey);
	CachedBBComp->ClearValue(TargetLastKnownLocationKey);
	CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);

	if (bClearCanAttack) SetCanAttackBB(false);
}

UNSEnemyThreatComponent* ANSEnemyDroneAIController::GetEnemyThreatComponent() const
{
	const APawn* P = GetPawn();
	return P ? P->FindComponentByClass<UNSEnemyThreatComponent>() : nullptr;
}

UNSEnemyAttackComponent* ANSEnemyDroneAIController::GetEnemyAttackComponent() const
{
	const APawn* P = GetPawn();
	return P ? P->FindComponentByClass<UNSEnemyAttackComponent>() : nullptr;
}

UNSEnemyTargetComponent* ANSEnemyDroneAIController::GetEnemyTargetComponent() const
{
	const APawn* P = GetPawn();
	return P ? P->FindComponentByClass<UNSEnemyTargetComponent>() : nullptr;	
}

UNSEnemyStateComponent* ANSEnemyDroneAIController::GetEnemyStateComponent() const
{
	const APawn* P = GetPawn();
	return P ? P->FindComponentByClass<UNSEnemyStateComponent>() : nullptr;
}

bool ANSEnemyDroneAIController::IsDroneAIBlocked() const
{
	if (const UNSEnemyStateComponent* State = GetEnemyStateComponent())
	{
		if (State->IsDead() || State->IsInactive()) return true;
	}
	if (IsControlledEnemyHitReacting()) return true;   // 베이스 헬퍼
	if (IsPhasePatternLocked())        return true;    // 베이스 헬퍼
	return false;
}

bool ANSEnemyDroneAIController::IsValidLivingTarget(const AActor* Target) const
{
	const UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent();
	return TargetComponent && TargetComponent->IsValidLivingTarget(Target);
}

ETeamAttitude::Type ANSEnemyDroneAIController::GetTeamAttitudeTo(const AActor& Other) const
{
	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!TeamAgent) return ETeamAttitude::Neutral;

	const FGenericTeamId OtherTeamId = TeamAgent->GetGenericTeamId();
	if (OtherTeamId == FGenericTeamId(static_cast<uint8>(ETeamId::Player)))
	{
		return IsValidLivingTarget(&Other) ? ETeamAttitude::Hostile : ETeamAttitude::Neutral;
	}
	if (OtherTeamId == FGenericTeamId(static_cast<uint8>(ETeamId::Enemy)))
	{
		return ETeamAttitude::Friendly;
	}
	return ETeamAttitude::Neutral;
}

