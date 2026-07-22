// Copyright 2026 One Team. All rights reserved.

#include "NSSTTask_ExecuteBossAbility.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSBossAbilityExecutorComponent.h"
#include "StateTreeExecutionContext.h"

FNSSTTask_ExecuteBossAbility::FNSSTTask_ExecuteBossAbility()
{
	// Ability 실행 완료를 StateTree Task Tick에서 계속 감시하도록 하는 설정
	bShouldCallTick = true;
}

EStateTreeRunStatus FNSSTTask_ExecuteBossAbility::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData = FInstanceDataType();

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
			Warning,
			TEXT("NSSTTask_ExecuteBossAbility: 공격 실행 요청 실패. FixedAttackId=%s"),
			*FixedAttackId.ToString());

		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CachedExecutor = Executor;
	InstanceData.RequestedAttackId = FixedAttackId;
	InstanceData.bRequestAccepted = true;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FNSSTTask_ExecuteBossAbility::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSBossAbilityExecutorComponent* Executor = InstanceData.CachedExecutor.Get();
	if (!Executor || !InstanceData.bRequestAccepted)
	{
		return EStateTreeRunStatus::Failed;
	}

	const ENSBossAbilityExecutionState ExecutionState = Executor->GetExecutionState();

	if (Executor->IsExecuting())
	{
		return EStateTreeRunStatus::Running;
	}

	if (ExecutionState == ENSBossAbilityExecutionState::Succeeded)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (ExecutionState == ENSBossAbilityExecutionState::Failed ||
		ExecutionState == ENSBossAbilityExecutionState::Cancelled)
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
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

	const bool bCompleted =
		Executor->WasLastAttackSuccessful() ||
		Executor->WasLastAttackFailed();

	if (bCancelAbilityOnExit && !bCompleted && Executor->IsExecuting())
	{
		Executor->CancelCurrentAttack();
	}

	InstanceData.CachedExecutor.Reset();
	InstanceData.RequestedAttackId = NAME_None;
	InstanceData.bRequestAccepted = false;
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
