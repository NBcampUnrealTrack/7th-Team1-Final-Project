// Copyright 2026 One Team. All rights reserved.

#include "NSSTTask_SetBossMode.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSBossModeComponent.h"
#include "StateTreeExecutionContext.h"

FNSSTTask_SetBossMode::FNSSTTask_SetBossMode()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FNSSTTask_SetBossMode::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	UNSBossModeComponent* ModeComponent = ResolveModeComponent(Context);
	if (!ModeComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("NSSTTask_SetBossMode: BossModeComponent를 찾지 못함"));
		return EStateTreeRunStatus::Failed;
	}

	if (!ModeTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("NSSTTask_SetBossMode: ModeTag가 비어 있음"));
		return EStateTreeRunStatus::Failed;
	}

	const bool bChanged = ModeComponent->SetMode(ModeTag);

	return bChanged
		       ? EStateTreeRunStatus::Running
		       : EStateTreeRunStatus::Failed;
}

void FNSSTTask_SetBossMode::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (!bClearModeOnExit || !ModeTag.IsValid())
	{
		return;
	}

	UNSBossModeComponent* ModeComponent = ResolveModeComponent(Context);
	if (!ModeComponent)
	{
		return;
	}

	if (ModeComponent->GetCurrentModeTag() == ModeTag)
	{
		ModeComponent->ClearMode();
	}
}

UNSBossModeComponent* FNSSTTask_SetBossMode::ResolveModeComponent(
	FStateTreeExecutionContext& Context) const
{
	UObject* Owner = Context.GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UNSBossModeComponent* ModeComponent = Cast<UNSBossModeComponent>(Owner))
	{
		return ModeComponent;
	}

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Pawn->FindComponentByClass<UNSBossModeComponent>();
	}

	if (AAIController* AIController = Cast<AAIController>(Owner))
	{
		APawn* Pawn = AIController->GetPawn();
		return Pawn
			       ? Pawn->FindComponentByClass<UNSBossModeComponent>()
			       : nullptr;
	}

	if (UActorComponent* OwnerComponent = Cast<UActorComponent>(Owner))
	{
		AActor* ComponentOwner = OwnerComponent->GetOwner();

		if (APawn* Pawn = Cast<APawn>(ComponentOwner))
		{
			return Pawn->FindComponentByClass<UNSBossModeComponent>();
		}

		if (AAIController* AIController = Cast<AAIController>(ComponentOwner))
		{
			APawn* Pawn = AIController->GetPawn();
			return Pawn
				       ? Pawn->FindComponentByClass<UNSBossModeComponent>()
				       : nullptr;
		}
	}

	return nullptr;
}
