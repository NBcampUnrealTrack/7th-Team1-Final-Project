// Copyright 2026 One Team. All rights reserved.

#include "NSSTTask_TitanWalkerMove.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/Combat/Component/NSTitanWalkerMoveComponent.h"
#include "StateTreeExecutionContext.h"

FNSSTTask_TitanWalkerMove::FNSSTTask_TitanWalkerMove()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FNSSTTask_TitanWalkerMove::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSTitanWalkerMoveComponent* MoveComponent = ResolveMoveComponent(Context);
	if (!MoveComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("NSSTTask_TitanWalkerMove: TitanWalkerMoveComponent를 찾지 못함"));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CachedMoveComponent = MoveComponent;

	AActor* TargetActor = ResolveTargetActor(Context, InstanceData);
	MoveComponent->SetMoveTarget(TargetActor);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FNSSTTask_TitanWalkerMove::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSTitanWalkerMoveComponent* MoveComponent = InstanceData.CachedMoveComponent.Get();
	if (!MoveComponent)
	{
		MoveComponent = ResolveMoveComponent(Context);
		InstanceData.CachedMoveComponent = MoveComponent;
	}

	if (!MoveComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* TargetActor = ResolveTargetActor(Context, InstanceData);
	MoveComponent->SetMoveTarget(TargetActor);
	MoveComponent->TickMove(DeltaTime);

	return EStateTreeRunStatus::Running;
}

void FNSSTTask_TitanWalkerMove::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UNSTitanWalkerMoveComponent* MoveComponent = InstanceData.CachedMoveComponent.Get();
	if (MoveComponent && InstanceData.bStopMoveOnExit)
	{
		MoveComponent->StopMove();
	}

	InstanceData.CachedMoveComponent.Reset();
}

UNSTitanWalkerMoveComponent* FNSSTTask_TitanWalkerMove::ResolveMoveComponent(
	FStateTreeExecutionContext& Context) const
{
	UObject* Owner = Context.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UNSTitanWalkerMoveComponent* MoveComponent =
		Cast<UNSTitanWalkerMoveComponent>(Owner))
	{
		return MoveComponent;
	}

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Pawn->FindComponentByClass<UNSTitanWalkerMoveComponent>();
	}

	if (AAIController* AIController = Cast<AAIController>(Owner))
	{
		APawn* Pawn = AIController->GetPawn();
		return Pawn
			       ? Pawn->FindComponentByClass<UNSTitanWalkerMoveComponent>()
			       : nullptr;
	}

	if (UActorComponent* OwnerComponent = Cast<UActorComponent>(Owner))
	{
		AActor* ComponentOwner = OwnerComponent->GetOwner();

		if (APawn* Pawn = Cast<APawn>(ComponentOwner))
		{
			return Pawn->FindComponentByClass<UNSTitanWalkerMoveComponent>();
		}

		if (AAIController* AIController = Cast<AAIController>(ComponentOwner))
		{
			APawn* Pawn = AIController->GetPawn();
			return Pawn
				       ? Pawn->FindComponentByClass<UNSTitanWalkerMoveComponent>()
				       : nullptr;
		}
	}

	return nullptr;
}

ANSBossAIController* FNSSTTask_TitanWalkerMove::ResolveBossController(
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

AActor* FNSSTTask_TitanWalkerMove::ResolveTargetActor(
	FStateTreeExecutionContext& Context,
	const FInstanceDataType& InstanceData) const
{
	if (IsValid(InstanceData.TargetActor))
	{
		return InstanceData.TargetActor;
	}

	ANSBossAIController* BossController = ResolveBossController(Context);
	return BossController ? BossController->GetCurrentTargetActor() : nullptr;
}
