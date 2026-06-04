// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterPoolManager.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"


ACharacter* UNSMonsterPoolManager::GetPooledMonster(
	UClass* CharacterClass,
	UNSEnemyData* EnemyData,
	const FVector& Location,
	const FRotator& Rotation)
{
	if (!GetWorld() || !CharacterClass || !EnemyData)
	{
		return nullptr;
	}

	// 데이터 에셋 기준 풀
	FNSMonsterPoolArray& Pool = PoolMap.FindOrAdd(EnemyData);

	// 풀 대기 액터 검색
	for (ACharacter* Candidate : Pool.Monsters)
	{
		ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(Candidate);
		if (Enemy && Enemy->IsInPool())
		{
			Enemy->SetNetDormancy(DORM_Awake);
			Enemy->PrepareForReuse(Location, Rotation);
			Enemy->FlushNetDormancy();
			return Enemy;
		}
	}

	// 풀 부족하면 신규 생성 (BeginPlay 전 데이터 주입)
	FTransform SpawnTransform(Rotation, Location);

	ANSEnemyCharacterBase* NewEnemy = GetWorld()->SpawnActorDeferred<ANSEnemyCharacterBase>(
		CharacterClass, SpawnTransform,
		nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (NewEnemy)
	{
		NewEnemy->SetEnemyData(EnemyData);
		NewEnemy->FinishSpawning(SpawnTransform);
		Pool.Monsters.Add(NewEnemy);
	}

	return NewEnemy;
}

void UNSMonsterPoolManager::ReturnMonsterToPool(ACharacter* Monster)
{
	if (!Monster)
	{
		return;
	}

	if (ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(Monster))
	{
		Enemy->DeactivateForPool();
		Enemy->SetNetDormancy(DORM_DormantAll);
	}
}
