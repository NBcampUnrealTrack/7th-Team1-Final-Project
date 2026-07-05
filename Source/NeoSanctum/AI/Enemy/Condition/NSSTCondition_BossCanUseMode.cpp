// NSSTCondition_BossCanUseMode.cpp

#include "NSSTCondition_BossCanUseMode.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "StateTreeExecutionContext.h"

bool FNSSTCondition_BossCanUseMode::TestCondition(
	FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.ModeTag.IsValid())
	{
		return false;
	}

	ANSBossAIController* BossController = ResolveBossController(Context);
	if (!BossController)
	{
		return false;
	}

	return BossController->CanUseAnyAttackInMode(InstanceData.ModeTag);
}

ANSBossAIController* FNSSTCondition_BossCanUseMode::ResolveBossController(
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
