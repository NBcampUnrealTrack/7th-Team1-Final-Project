// Copyright 2026 One Team. All rights reserved.

#include "NSSTTask_ExecuteBossAbility.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSBossAbilityExecutorComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FNSSTTask_ExecuteBossAbility::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSBossAbilityExecutorComponent* Executor = ResolveExecutor(Context);
	if (!Executor)
	{
		UE_LOG(LogTemp, Warning, TEXT("NSSTTask_ExecuteBossAbility: BossAbilityExecutorComponent를 찾지 못함"));
		return EStateTreeRunStatus::Failed;
	}

	const bool bRequested = Executor->RequestAttack(FixedAttackId);
	if (!bRequested)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("NSSTTask_ExecuteBossAbility: 공격 실행 요청 실패. FixedAttackId=%s"),
			*FixedAttackId.ToString());

		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CachedExecutor = Executor;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FNSSTTask_ExecuteBossAbility::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSBossAbilityExecutorComponent* Executor = InstanceData.CachedExecutor.Get();
	if (!Executor)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Executor->IsExecuting())
	{
		return EStateTreeRunStatus::Running;
	}

	if (Executor->WasLastAttackSuccessful())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}

void FNSSTTask_ExecuteBossAbility::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSBossAbilityExecutorComponent* Executor = InstanceData.CachedExecutor.Get();
	if (!Executor)
	{
		return;
	}

	if (bCancelAbilityOnExit && Executor->IsExecuting())
	{
		Executor->CancelCurrentAttack();
	}

	InstanceData.CachedExecutor.Reset();
}

UNSBossAbilityExecutorComponent* FNSSTTask_ExecuteBossAbility::ResolveExecutor(
	FStateTreeExecutionContext& Context) const
{
	UObject* Owner = Context.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UNSBossAbilityExecutorComponent* Executor =
		Cast<UNSBossAbilityExecutorComponent>(Owner))
	{
		return Executor;
	}

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Pawn->FindComponentByClass<UNSBossAbilityExecutorComponent>();
	}

	if (AAIController* AIController = Cast<AAIController>(Owner))
	{
		APawn* Pawn = AIController->GetPawn();
		return Pawn
			       ? Pawn->FindComponentByClass<UNSBossAbilityExecutorComponent>()
			       : nullptr;
	}

	if (UActorComponent* OwnerComponent = Cast<UActorComponent>(Owner))
	{
		AActor* ComponentOwner = OwnerComponent->GetOwner();

		if (APawn* Pawn = Cast<APawn>(ComponentOwner))
		{
			return Pawn->FindComponentByClass<UNSBossAbilityExecutorComponent>();
		}

		if (AAIController* AIController = Cast<AAIController>(ComponentOwner))
		{
			APawn* Pawn = AIController->GetPawn();
			return Pawn
				       ? Pawn->FindComponentByClass<UNSBossAbilityExecutorComponent>()
				       : nullptr;
		}
	}

	return nullptr;
}
