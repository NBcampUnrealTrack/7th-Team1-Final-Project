// Copyright 2026 One Team. All rights reserved.

#include "NSSTCondition_BossCanUseAttack.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "StateTreeExecutionContext.h"

bool FNSSTCondition_BossCanUseAttack::TestCondition(
	FStateTreeExecutionContext& Context) const
{
	if (AttackId.IsNone())
	{
		return false;
	}

	ANSBossAIController* BossController = ResolveBossController(Context);
	if (!BossController)
	{
		return false;
	}

	return BossController->CanUseAttackById(AttackId);
}

ANSBossAIController* FNSSTCondition_BossCanUseAttack::ResolveBossController(
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
