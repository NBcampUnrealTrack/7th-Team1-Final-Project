// Copyright 2026 One Team. All rights reserved.

#include "NSEnvQueryContext_EnemyTarget.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"

void UNSEnvQueryContext_EnemyTarget::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	const ANSEnemyAIController* EnemyController = Cast<ANSEnemyAIController>(QueryOwner);

	if (!EnemyController)
	{
		if (const APawn* Pawn = Cast<APawn>(QueryOwner))
		{
			EnemyController = Cast<ANSEnemyAIController>(Pawn->GetController());
		}
	}

	if (EnemyController)
	{
		if (AActor* TargetActor = EnemyController->GetCurrentTargetActor())
		{
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
		}
	}
}
