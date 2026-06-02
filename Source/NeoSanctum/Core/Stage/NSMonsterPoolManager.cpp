// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterPoolManager.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"


ACharacter* UNSMonsterPoolManager::GetPooledMonster(UClass* MonsterClass, const FVector& Location, const FRotator& Rotation)
{
	if (!GetWorld() || !MonsterClass)
	{
		return nullptr;
	}

	FNSMonsterPoolArray& Pool = PoolMap.FindOrAdd(MonsterClass);

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

	// 풀 부족하면 신규 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACharacter* NewActor = GetWorld()->SpawnActor<ACharacter>(
		MonsterClass, FTransform(Rotation, Location), SpawnParams);

	if (NewActor)
	{
		Pool.Monsters.Add(NewActor);
	}

	return NewActor;
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