// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSMonsterSpawnType.generated.h"


class ANSEnemyCharacterBase;
class UNSEnemyData;

USTRUCT(BlueprintType)
struct FNSMonsterSpawnRow : public FTableRowBase
{
	GENERATED_BODY()
	
	// 캐릭터 클래스 (지상/공중)
	UPROPERTY(EditDefaultsOnly, Category = "SpawnData")
	TSoftClassPtr<ANSEnemyCharacterBase> CharacterClass;

	// 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category = "SpawnData")
	TSoftObjectPtr<UNSEnemyData> EnemyData;

	UPROPERTY(EditDefaultsOnly, Category = "SpawnData")
	float SpawnWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SpawnData")
	int32 MinQuantity = 10;

	UPROPERTY(EditDefaultsOnly, Category = "SpawnData")
	int32 MaxQuantity = 20;
};
