// Copyright 2026 One Team. All rights reserved.

#include "NSBossAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "NeoSanctum/Combat/Component/NSBossTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

ANSBossAIController::ANSBossAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this,
		&ANSBossAIController::OnTargetPerceptionUpdated);
}

void ANSBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent())
	{
		AttackComponent->ResetAttackState();
	}

	if (UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent())
	{
		PhaseComponent->ResetPhaseState();
	}

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ResetThreatState();
	}

	if (UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent())
	{
		BossTargetComponent->ResetTargets();
	}

	if (UNSBossModeComponent* BossModeComponent = GetBossModeComponent())
	{
		BossModeComponent->InitializeMode();
	}

	const UNSEnemyData* EnemyData = GetControlledEnemyData();
	if (!EnemyData)
	{
		return;
	}

	StartEnemyBrain(EnemyData);

	InitBBState();
	ResetTargetingState();

	UpdateEnemyPhase();
	SyncModeBlackboard();
}

void ANSBossAIController::OnUnPossess()
{
	StopEnemyBrain(TEXT("Boss UnPossess"));

	ResetTargetingState();

	if (UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent())
	{
		AttackComponent->ResetAttackState();
	}

	if (UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent())
	{
		PhaseComponent->ResetPhaseState();
	}

	CachedBBComp = nullptr;

	Super::OnUnPossess();
}

void ANSBossAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateEnemyPhase();
	SyncModeBlackboard();

	if (IsBossAIBlocked())
	{
		StopMovement();
		ClearAttackState();
		return;
	}

	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		if (ThreatComponent->CanEvaluateTarget())
		{
			UpdateTargetSelection();
			ThreatComponent->ScheduleNextEvaluation();
		}
	}

	UpdateCurrentTargetBlackboard();
	CanUseAnyAttackByDistance();
}

ETeamAttitude::Type ANSBossAIController::GetTeamAttitudeTo(const AActor& Other) const
{
	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!TeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	const FGenericTeamId OtherTeamId = TeamAgent->GetGenericTeamId();

	if (OtherTeamId == FGenericTeamId(static_cast<uint8>(ETeamId::Player)))
	{
		return IsValidLivingTarget(&Other)
			       ? ETeamAttitude::Hostile
			       : ETeamAttitude::Neutral;
	}

	if (OtherTeamId == FGenericTeamId(static_cast<uint8>(ETeamId::Enemy)))
	{
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Neutral;
}

void ANSBossAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	if (GetTeamAttitudeTo(*Actor) != ETeamAttitude::Hostile)
	{
		return;
	}

	UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	if (!ThreatComponent)
	{
		return;
	}

	if (!IsValidLivingTarget(Actor))
	{
		const bool bWasCurrentTarget = GetCurrentTargetActor() == Actor;

		ThreatComponent->RemoveTarget(Actor, false);

		if (bWasCurrentTarget)
		{
			ClearCurrentCombatTarget(false);
		}
		else
		{
			UpdateCurrentTargetBlackboard();
		}

		return;
	}

	ThreatComponent->UpdateThreatFromStimulus(Actor, Stimulus);
	UpdateTargetSelection();
}

bool ANSBossAIController::IsBossAIBlocked() const
{
	const UNSEnemyStateComponent* StateComponent = GetEnemyStateComponent();

	if (StateComponent)
	{
		if (StateComponent->IsDead() || StateComponent->IsInactive())
		{
			return true;
		}
	}

	if (IsControlledEnemyHitReacting())
	{
		return true;
	}

	if (IsPhasePatternLocked())
	{
		return true;
	}

	return false;
}

bool ANSBossAIController::IsValidLivingTarget(const AActor* Target) const
{
	const UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent();
	return TargetComponent && TargetComponent->IsValidLivingTarget(Target);
}

bool ANSBossAIController::CanMaintainCurrentTarget(AActor* TargetActor) const
{
	if (!IsValidLivingTarget(TargetActor))
	{
		return false;
	}

	const UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent();
	if (!TargetComponent)
	{
		return false;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = TargetComponent->ResolveAttackActor(
		TargetActor,
		bHasDirectLineOfSight);

	return IsValid(AttackActor);
}

void ANSBossAIController::UpdateTargetSelection()
{
	UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	if (!ThreatComponent)
	{
		return;
	}

	AActor* CurrentTarget = ThreatComponent->GetCurrentTarget();

	const bool bIsAttacking =
		CachedBBComp && CachedBBComp->GetValueAsBool(IsAttackingKey);

	const bool bCanMaintainCurrentTarget =
		CurrentTarget && CanMaintainCurrentTarget(CurrentTarget);

	const FNSEnemyThreatUpdateResult Result =
		ThreatComponent->UpdateTarget(
			bIsAttacking,
			bCanMaintainCurrentTarget);

	if (Result.bTargetChanged)
	{
		ClearAttackState();
	}

	UpdateCurrentTargetBlackboard();
}

void ANSBossAIController::ResetTargetingState()
{
	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ResetThreatState();
	}

	if (UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent())
	{
		BossTargetComponent->ResetTargets();
	}

	CurrentAttackActor.Reset();

	ClearTargetBB(true);
}

void ANSBossAIController::ClearCurrentCombatTarget(bool bBlockReacquisition)
{
	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->ClearCurrentTarget(bBlockReacquisition);
	}

	if (UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent())
	{
		BossTargetComponent->ResetTargets();
	}

	CurrentAttackActor.Reset();

	ClearTargetBB(true);
}

void ANSBossAIController::UpdateCurrentTargetBlackboard()
{
	if (!CachedBBComp)
	{
		return;
	}

	AActor* TargetActor = GetCurrentTargetActor();

	if (!IsValidLivingTarget(TargetActor))
	{
		ClearTargetBB(false);
		return;
	}

	CachedBBComp->SetValueAsObject(TargetActorKey, TargetActor);

	FVector LastKnownLocation = TargetActor->GetActorLocation();

	if (const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->TryGetLastKnownLocation(TargetActor, LastKnownLocation);
	}

	CachedBBComp->SetValueAsVector(TargetLastKnownLocationKey, LastKnownLocation);

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = nullptr;

	if (const UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent())
	{
		AttackActor = TargetComponent->ResolveAttackActor(
			TargetActor,
			bHasDirectLineOfSight);
	}

	SetAttackActorState(AttackActor);
	CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, bHasDirectLineOfSight);
}

void ANSBossAIController::SyncModeBlackboard()
{
	if (!CachedBBComp)
	{
		return;
	}

	const UNSBossModeComponent* BossModeComponent = GetBossModeComponent();
	if (!BossModeComponent)
	{
		CachedBBComp->ClearValue(CurrentModeTagKey);
		return;
	}

	const FGameplayTag CurrentModeTag = BossModeComponent->GetCurrentModeTag();

	if (CurrentModeTag.IsValid())
	{
		CachedBBComp->SetValueAsName(CurrentModeTagKey, CurrentModeTag.GetTagName());
	}
	else
	{
		CachedBBComp->ClearValue(CurrentModeTagKey);
	}
}

bool ANSBossAIController::CanUseAnyAttackByDistance()
{
	UpdateEnemyPhase();

	if (IsBossAIBlocked())
	{
		SetCanAttackBB(false);
		return false;
	}

	const FNSEnemyAttackRow* UsableAttack = FindAttackRowByDistance(false);

	SetCanAttackBB(UsableAttack != nullptr);

	return UsableAttack != nullptr;
}

const FNSEnemyAttackRow* ANSBossAIController::GetAttackRowByDistance()
{
	UpdateEnemyPhase();

	if (IsBossAIBlocked())
	{
		SetCanAttackBB(false);
		return nullptr;
	}

	const FNSEnemyAttackRow* SelectedAttack = FindAttackRowByDistance(true);

	SetCanAttackBB(SelectedAttack != nullptr);

	return SelectedAttack;
}

const FNSEnemyAttackRow* ANSBossAIController::FindAttackRowByDistance(bool bSelectWeightedAttack)
{
	APawn* BossPawn = GetPawn();
	AActor* TargetActor = GetCurrentTargetActor();

	if (!BossPawn || !IsValidLivingTarget(TargetActor))
	{
		ClearAttackState();
		return nullptr;
	}

	UNSEnemyTargetComponent* TargetComponent = GetEnemyTargetComponent();
	UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent();

	if (!TargetComponent || !AttackComponent)
	{
		ClearAttackState();
		return nullptr;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = TargetComponent->ResolveAttackActor(
		TargetActor,
		bHasDirectLineOfSight);

	if (!IsValid(AttackActor))
	{
		ClearAttackState();
		return nullptr;
	}

	const float Distance =
		FVector::Dist(BossPawn->GetActorLocation(), TargetActor->GetActorLocation());

	const FNSEnemyAttackRow* SelectedAttack =
		AttackComponent->SelectAttack(
			TargetActor,
			AttackActor,
			Distance,
			bHasDirectLineOfSight,
			bSelectWeightedAttack);

	if (!SelectedAttack)
	{
		ClearAttackState();
		return nullptr;
	}

	if (!BuildAttackTargetsForRow(*SelectedAttack))
	{
		ClearAttackState();
		return nullptr;
	}

	SetAttackActorState(AttackActor);

	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, bHasDirectLineOfSight);
	}

	return SelectedAttack;
}

bool ANSBossAIController::BuildAttackTargetsForRow(const FNSEnemyAttackRow& AttackRow)
{
	UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent();
	if (!BossTargetComponent)
	{
		return false;
	}

	BossTargetComponent->BuildAttackTargets(
		GetCurrentTargetActor(),
		AttackRow);

	return BossTargetComponent->GetAttackTargetCount() > 0;
}

void ANSBossAIController::RecordAttackUsed(const FNSEnemyAttackRow& AttackRow)
{
	if (UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent())
	{
		ThreatComponent->NotifyAttackStarted();
	}

	if (UNSEnemyAttackComponent* AttackComponent = GetEnemyAttackComponent())
	{
		AttackComponent->RecordAttackUsed(AttackRow);
	}

	SetIsAttackingBB(true);
}

AActor* ANSBossAIController::GetCurrentTargetActor() const
{
	const UNSEnemyThreatComponent* ThreatComponent = GetEnemyThreatComponent();
	return ThreatComponent ? ThreatComponent->GetCurrentTarget() : nullptr;
}

AActor* ANSBossAIController::GetCurrentAttackActor() const
{
	return CurrentAttackActor.IsValid()
		       ? CurrentAttackActor.Get()
		       : nullptr;
}

void ANSBossAIController::GetCurrentAttackTargets(TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	const UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent();
	if (!BossTargetComponent)
	{
		return;
	}

	BossTargetComponent->GetCurrentAttackTargets(OutTargets);
}

void ANSBossAIController::SetAttackActorState(AActor* AttackActor)
{
	if (IsValid(AttackActor))
	{
		CurrentAttackActor = AttackActor;
	}
	else
	{
		CurrentAttackActor.Reset();
	}

	if (!CachedBBComp)
	{
		return;
	}

	if (IsValid(AttackActor))
	{
		CachedBBComp->SetValueAsObject(AttackActorKey, AttackActor);
	}
	else
	{
		CachedBBComp->ClearValue(AttackActorKey);
	}
}

void ANSBossAIController::SetCanAttackBB(bool bCanAttack)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(CanAttackKey, bCanAttack);
	}
}

void ANSBossAIController::SetIsAttackingBB(bool bIsAttacking)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsAttackingKey, bIsAttacking);
	}
}

void ANSBossAIController::ClearAttackState()
{
	SetAttackActorState(nullptr);

	if (UNSBossTargetComponent* BossTargetComponent = GetBossTargetComponent())
	{
		BossTargetComponent->ResetTargets();
	}

	SetCanAttackBB(false);
	SetIsAttackingBB(false);
}

void ANSBossAIController::ClearTargetBB(bool bClearCanAttack)
{
	SetAttackActorState(nullptr);

	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(TargetActorKey);
	CachedBBComp->ClearValue(AttackActorKey);
	CachedBBComp->ClearValue(TargetLastKnownLocationKey);
	CachedBBComp->SetValueAsBool(HasTargetLineOfSightKey, false);

	if (bClearCanAttack)
	{
		SetCanAttackBB(false);
	}
}

void ANSBossAIController::InitBBState()
{
	if (!CachedBBComp)
	{
		return;
	}

	ClearAttackState();
	ClearTargetBB(false);

	CachedBBComp->SetValueAsBool(PhasePatternLockedKey, false);
	CachedBBComp->SetValueAsName(CurrentPhaseIdKey, NAME_None);
	CachedBBComp->ClearValue(CurrentModeTagKey);
}

UNSEnemyAttackComponent* ANSBossAIController::GetEnemyAttackComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyAttackComponent>()
		       : nullptr;
}

UNSEnemyTargetComponent* ANSBossAIController::GetEnemyTargetComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyTargetComponent>()
		       : nullptr;
}

UNSEnemyThreatComponent* ANSBossAIController::GetEnemyThreatComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyThreatComponent>()
		       : nullptr;
}

UNSEnemyStateComponent* ANSBossAIController::GetEnemyStateComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSEnemyStateComponent>()
		       : nullptr;
}

UNSBossModeComponent* ANSBossAIController::GetBossModeComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSBossModeComponent>()
		       : nullptr;
}

UNSBossTargetComponent* ANSBossAIController::GetBossTargetComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSBossTargetComponent>()
		       : nullptr;
}
