// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSMonsterPoolManager.generated.h"

class UNSEnemyData;

USTRUCT()
struct FNSMonsterPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> Monsters;
};

UCLASS()
class NEOSANCTUM_API UNSMonsterPoolManager : public UObject
{
	GENERATED_BODY()
	
public:
	ACharacter* GetPooledMonster(
		UClass* CharacterClass,
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation,
		const FNSDifficultyScale& Scale);

	void ReturnMonsterToPool(ACharacter* Monster);

private:
	// 풀 키 = 데이터 에셋 (종류별 독립 풀)
	UPROPERTY(Transient)
	TMap<TObjectPtr<UNSEnemyData>, FNSMonsterPoolArray> PoolMap;
	
};
