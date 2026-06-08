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
	
	// 스포너 에디터에서 On Actor Enter Room 이벤트에 연결할 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void OnActorEnteredRoom(AActor* OtherActor);
	
	// 스포너 에디터에서 On Actor Exit Room 이벤트에 연결할 함수
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void OnActorExitedRoom(AActor* OtherActor);

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ActivateSpawner();
	
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ReturnMonstersToPool();
	
	// 배치된 위치를 받아서 최소 간격 이상으로 새 위치 선출
	FVector GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced) const;
	
	// 인스턴스마다 어떤 몬스터 스폰할지 정하는 용도
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<UDataTable> SpawnDataTable;

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
	// 스폰 범위용
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnRadius = 200.0f;

	// 적 사이 최소 간격
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float MinSpawnSpacing = 90.0f;

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
	
	// 룸에 남아있는 플레이어 숫자 체크용
	int32 PlayersInRoom = 0;
};
