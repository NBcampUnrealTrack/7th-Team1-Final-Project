// Copyright 2026 One Team. All rights reserved.

#include "NSEnvQueryContext_OtherEnemies.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

void UNSEnvQueryContext_OtherEnemies::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	const APawn* QuerierPawn = Cast<APawn>(QueryOwner);

	if (!QuerierPawn)
	{
		if (const AAIController* AIController = Cast<AAIController>(QueryOwner))
		{
			QuerierPawn = AIController->GetPawn();
		}
	}

	UWorld* World = QueryOwner ? QueryOwner->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TArray<AActor*> OtherEnemies;
	for (TActorIterator<ANSEnemyCharacterBase> It(World); It; ++It)
	{
		ANSEnemyCharacterBase* Enemy = *It;
		if (Enemy != QuerierPawn && !Enemy->IsDead() && !Enemy->IsInPool())
		{
			OtherEnemies.Add(Enemy);
		}
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, OtherEnemies);
}
