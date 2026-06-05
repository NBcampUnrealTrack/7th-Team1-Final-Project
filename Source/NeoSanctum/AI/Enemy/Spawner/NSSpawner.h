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
	
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ReturnMonstersToPool();
	
	// 룸 바운드 조회용
	bool GetRoomBounds(FVector& OutCenter, FVector& OutExtent) const;
	
	// 배치된 위치를 받아서 최소 간격 이상으로 새 위치 선출
	FVector GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced) const;

private:
	void ProcessSpawnProbability(UDataTable* SpawnTable);
	void RequestAsyncLoad();
	void OnLoadCompleted();
	void ExecuteFinalSpawn();
	
	bool bHasSpawned = false;
	
	// 스포너가 스폰한 몬스터 저장용
	UPROPERTY()
	TArray<TObjectPtr<ANSEnemyCharacterBase>> SpawnedMonsters;

protected:
	// 룸 바운드를 못 찾았을 때만 쓰는 폴백용 반경(정상 경로에선 미사용)
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float FallbackSpawnRadius = 300.0f;

	// 적 사이 최소 간격
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float MinSpawnSpacing = 200.0f;

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
