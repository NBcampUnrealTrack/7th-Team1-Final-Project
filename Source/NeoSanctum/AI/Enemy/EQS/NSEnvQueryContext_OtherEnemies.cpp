// Copyright 2026 One Team. All rights reserved.

#include "NSEnvQueryContext_OtherEnemies.h"

#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

void UNSEnvQueryContext_OtherEnemies::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	const ANSEnemyAIController* QueryController = Cast<ANSEnemyAIController>(QueryOwner);
	const ANSEnemyCharacterBase* QuerierEnemy = Cast<ANSEnemyCharacterBase>(QueryOwner);

	if (!QueryController && QuerierEnemy)
	{
		QueryController = Cast<ANSEnemyAIController>(QuerierEnemy->GetController());
	}

	if (!QuerierEnemy && QueryController)
	{
		QuerierEnemy = Cast<ANSEnemyCharacterBase>(QueryController->GetPawn());
	}

	if (!QueryController || !QuerierEnemy)
	{
		return;
	}

	AActor* SharedTarget = QueryController->GetCurrentTargetActor();

	if (!IsValid(SharedTarget))
	{
		return;
	}

	const UNSEnemyData* EnemyData = QuerierEnemy->GetEnemyData();

	const float NeighborRadius = EnemyData ? EnemyData->MeleeEQSNeighborRadius : 1000.0f;
	const float NeighborRadiusSquared = FMath::Square(NeighborRadius);

	UWorld* World = QueryOwner ? QueryOwner->GetWorld() : nullptr;

	if (!World)
	{
		return;
	}

	TArray<AActor*> OtherEnemies;

	for (TActorIterator<ANSEnemyCharacterBase> It(World); It; ++It)
	{
		ANSEnemyCharacterBase* OtherEnemy = *It;

		if (OtherEnemy == QuerierEnemy ||
			OtherEnemy->IsDead() ||
			OtherEnemy->IsInPool())
		{
			continue;
		}

		const ANSEnemyAIController* OtherController = Cast<ANSEnemyAIController>(OtherEnemy->GetController());

		if (!OtherController ||
			OtherController->GetCurrentTargetActor() != SharedTarget)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			OtherEnemy->GetActorLocation(),
			SharedTarget->GetActorLocation());

		if (DistanceSquared > NeighborRadiusSquared)
		{
			continue;
		}

		OtherEnemies.Add(OtherEnemy);
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, OtherEnemies);
}
