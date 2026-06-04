// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NSSpawner.generated.h"


class UDataTable;
class ANSEnemyCharacterBase;
class UNSEnemyData;

UCLASS()
class NEOSANCTUM_API ANSSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANSSpawner();

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ActivateSpawner(UDataTable* SpawnTable);

private:
	void ProcessSpawnProbability(UDataTable* SpawnTable);
	void RequestAsyncLoad();
	void OnLoadCompleted();
	void ExecuteFinalSpawn();

	FVector GetRandomSpawnLocation() const;

protected:
	// 스폰 분산 반경
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnRadius = 300.0f;

	// 로드 실패 시 폴백용
	UPROPERTY(EditDefaultsOnly, Category = "Fallback")
	TSubclassOf<ANSEnemyCharacterBase> FallbackCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Fallback")
	TObjectPtr<UNSEnemyData> FallbackEnemyData;

private:
	TSoftClassPtr<ANSEnemyCharacterBase> SoftCharacterClass;
	TSoftObjectPtr<UNSEnemyData> SoftEnemyData;
	int32 FinalSpawnQuantity = 0;
	TSharedPtr<FStreamableHandle> StreamingHandle;
};
