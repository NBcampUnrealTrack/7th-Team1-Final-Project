// Copyright 2026 One Team. All rights reserved.

#include "NSSTEvaluator_BossCombat.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "StateTreeExecutionContext.h"

void FNSSTEvaluator_BossCombat::TreeStart(
	FStateTreeExecutionContext& Context) const
{
	UpdateInstanceData(Context);
}

void FNSSTEvaluator_BossCombat::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	UpdateInstanceData(Context);
}

void FNSSTEvaluator_BossCombat::UpdateInstanceData(
	FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData = FInstanceDataType();

	ANSBossAIController* BossController = ResolveBossController(Context);
	APawn* BossPawn = ResolveBossPawn(Context, BossController);

	if (!BossController || !BossPawn)
	{
		return;
	}

	const UNSEnemyStateComponent* StateComponent =
		BossPawn->FindComponentByClass<UNSEnemyStateComponent>();

	const UNSEnemyPhaseComponent* PhaseComponent =
		BossPawn->FindComponentByClass<UNSEnemyPhaseComponent>();

	const UNSBossModeComponent* ModeComponent =
		BossPawn->FindComponentByClass<UNSBossModeComponent>();

	const UNSEnemyTargetComponent* TargetComponent =
		BossPawn->FindComponentByClass<UNSEnemyTargetComponent>();

	const UNSBossAbilityExecutorComponent* ExecutorComponent =
		BossPawn->FindComponentByClass<UNSBossAbilityExecutorComponent>();

	InstanceData.TargetActor = BossController->GetCurrentTargetActor();
	InstanceData.bHasTarget =
		TargetComponent &&
		TargetComponent->IsValidLivingTarget(InstanceData.TargetActor);

	if (InstanceData.bHasTarget && TargetComponent)
	{
		bool bHasDirectLineOfSight = false;
		AActor* AttackActor = TargetComponent->ResolveAttackActor(
			InstanceData.TargetActor,
			bHasDirectLineOfSight);

		InstanceData.AttackActor = AttackActor;
		InstanceData.bHasAttackActor = IsValid(AttackActor);
		InstanceData.bHasLineOfSight = bHasDirectLineOfSight;
		InstanceData.TargetDistance =
			FVector::Dist(BossPawn->GetActorLocation(), InstanceData.TargetActor->GetActorLocation());
	}

	if (StateComponent)
	{
		InstanceData.bIsDead = StateComponent->IsDead();
		InstanceData.bIsInactive = StateComponent->IsInactive();
		InstanceData.bIsHitReacting = StateComponent->IsHitReacting();
	}

	if (PhaseComponent)
	{
		InstanceData.bIsPhaseLocked = PhaseComponent->IsPatternLocked();
		InstanceData.CurrentPhaseId = PhaseComponent->GetCurrentPhaseId();
		InstanceData.CurrentPhaseTag = PhaseComponent->GetCurrentPhaseTag();
	}

	if (ModeComponent)
	{
		InstanceData.CurrentModeTag = ModeComponent->GetCurrentModeTag();
	}

	if (ExecutorComponent)
	{
		InstanceData.ExecutionState = ExecutorComponent->GetExecutionState();
		InstanceData.CurrentAttackId = ExecutorComponent->GetCurrentAttackId();
		InstanceData.bIsAttacking = ExecutorComponent->IsExecuting();
	}
	else
	{
		InstanceData.bIsAttacking = BossController->IsBossAttackInProgress();
	}

	InstanceData.bCanAct =
		!InstanceData.bIsDead &&
		!InstanceData.bIsInactive &&
		!InstanceData.bIsHitReacting &&
		!InstanceData.bIsPhaseLocked &&
		!InstanceData.bIsAttacking;

	InstanceData.bCanSelectAttack =
		InstanceData.bCanAct &&
		InstanceData.bHasTarget &&
		InstanceData.bHasAttackActor;
}

ANSBossAIController* FNSSTEvaluator_BossCombat::ResolveBossController(
	FStateTreeExecutionContext& Context) const
{
	UObject* Owner = Context.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (ANSBossAIController* BossController = Cast<ANSBossAIController>(Owner))
	{
		return BossController;
	}

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Cast<ANSBossAIController>(Pawn->GetController());
	}

	if (UActorComponent* OwnerComponent = Cast<UActorComponent>(Owner))
	{
		AActor* ComponentOwner = OwnerComponent->GetOwner();

		if (ANSBossAIController* BossController = Cast<ANSBossAIController>(ComponentOwner))
		{
			return BossController;
		}

		if (APawn* Pawn = Cast<APawn>(ComponentOwner))
		{
			return Cast<ANSBossAIController>(Pawn->GetController());
		}
	}

	return nullptr;
}

APawn* FNSSTEvaluator_BossCombat::ResolveBossPawn(
	FStateTreeExecutionContext& Context,
	ANSBossAIController* BossController) const
{
	if (BossController)
	{
		return BossController->GetPawn();
	}

	UObject* Owner = Context.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Pawn;
	}

	if (UActorComponent* OwnerComponent = Cast<UActorComponent>(Owner))
	{
		return Cast<APawn>(OwnerComponent->GetOwner());
	}

	return nullptr;
}
