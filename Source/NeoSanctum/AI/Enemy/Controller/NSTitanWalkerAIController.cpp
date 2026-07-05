// Copyright 2026 One Team. All rights reserved.

#include "NSTitanWalkerAIController.h"

#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSBossAbilityExecutorComponent.h"

ANSTitanWalkerAIController::ANSTitanWalkerAIController()
{
}

bool ANSTitanWalkerAIController::IsBossAttackInProgress() const
{
	if (Super::IsBossAttackInProgress())
	{
		return true;
	}

	const UNSBossAbilityExecutorComponent* ExecutorComponent = GetBossAbilityExecutorComponent();

	return ExecutorComponent && ExecutorComponent->IsExecuting();
}

UNSBossAbilityExecutorComponent* ANSTitanWalkerAIController::GetBossAbilityExecutorComponent() const
{
	const APawn* ControlledPawn = GetPawn();

	return ControlledPawn
		       ? ControlledPawn->FindComponentByClass<UNSBossAbilityExecutorComponent>()
		       : nullptr;
}
