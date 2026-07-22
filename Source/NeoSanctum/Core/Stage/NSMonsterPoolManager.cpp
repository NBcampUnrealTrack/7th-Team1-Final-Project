// Copyright 2026 One Team. All rights reserved.


#include "NSMonsterPoolManager.h"
#include "GameFramework/Character.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"


ACharacter* UNSMonsterPoolManager::GetPooledMonster(
	UClass* CharacterClass,
	UNSEnemyData* EnemyData,
	const FVector& Location,
	const FRotator& Rotation,
	const FNSDifficultyScale& Scale)
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
			Enemy->SetDifficultyScale(Scale); 
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
		NewEnemy->SetDifficultyScale(Scale); 
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
		Enemy->FlushNetDormancy();
	}
}

void UNSMonsterPoolManager::PrewarmBegin(
	const TArray<FNSPrewarmRequest>& Requests,
	int32 PerTickCount, 
	FSimpleDelegate OnComplete)
{
	PrewarmQueue = Requests;
	PrewarmQueueIndex = 0;
	PrewarmMadeInCurrent = 0;
	PrewarmPerTick = FMath::Max(1, PerTickCount);
	PrewarmOnComplete = OnComplete;

	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] PrewarmBegin Requests=%d"), Requests.Num());
}

bool UNSMonsterPoolManager::PrewarmStep()
{
	int32 MadeThisTick = 0;

	while (MadeThisTick < PrewarmPerTick && PrewarmQueueIndex < PrewarmQueue.Num())
	{
		const FNSPrewarmRequest& Req = PrewarmQueue[PrewarmQueueIndex];

		if (Req.CharacterClass && Req.EnemyData && Req.Count > 0)
		{
			// 재사용 시 GetPooledMonster에서 재주입
			FNSDifficultyScale DummyScale; 
			if (ACharacter* Made = GetPooledMonster(
				Req.CharacterClass,
				Req.EnemyData,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				DummyScale))
			{
				ReturnMonsterToPool(Made);
			}
		}

		++PrewarmMadeInCurrent;
		++MadeThisTick;

		if (PrewarmMadeInCurrent >= Req.Count)
		{
			++PrewarmQueueIndex;
			PrewarmMadeInCurrent = 0;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Prewarm] 진행 Queue=%d/%d"),
		PrewarmQueueIndex, PrewarmQueue.Num());

	if (PrewarmQueueIndex >= PrewarmQueue.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Prewarm] 완료"));
		PrewarmOnComplete.ExecuteIfBound();
		
		return false; 
	}
	return true; 
}


UWorld* UNSMonsterPoolManager::GetWorld() const
{
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	
	return nullptr;
}


