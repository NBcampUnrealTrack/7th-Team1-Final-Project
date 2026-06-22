// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "NSSpawner.generated.h"


class UDataTable;
class ANSEnemyCharacterBase;
class UNSEnemyData;
class URoom;
class USphereComponent;

UCLASS()
class NEOSANCTUM_API ANSSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANSSpawner();
	
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	URoom* GetOwningRoom();

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void EvaluateRelevancy(int32 MinRelevancy);
	
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ActivateSpawner();
	
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ReturnMonstersToPool();
	
	// 배치된 위치를 받아서 최소 간격 이상으로 새 위치 선출
	FVector GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced) const;
	
	// 인스턴스마다 어떤 몬스터 스폰할지 정하는 용도
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
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
	virtual void BeginPlay() override;
	// 스폰 범위용
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	float SpawnRadius = 200.0f;
	
	// 적 사이 최소 간격
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	float MinSpawnSpacing = 90.0f;
	
	// 런타임(Standalone 포함) 스폰 반경 디버그 표시용
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	bool bShowRuntimeSpawnRadius = false;
	
	// 스포너별 디버그 스피어 색상 (인스턴스마다 다르게 지정 가능)
	UPROPERTY(EditAnywhere, Category = "SpawnerSet", meta = (EditCondition = "bShowRuntimeSpawnRadius"))
	FColor DebugSphereColor = FColor::Green;
	
	// 스폰 반경 시각화용(에디터 전용)
	UPROPERTY(VisibleAnywhere, Category = "SpawnerSet")
	TObjectPtr<USphereComponent> SpawnRadiusVisualizer;

	// 로드 실패 시 폴백용
	UPROPERTY(EditDefaultsOnly, Category = "Fallback")
	TSubclassOf<ANSEnemyCharacterBase> FallbackCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Fallback")
	TObjectPtr<UNSEnemyData> FallbackEnemyData;
	
	// 스폰 타이밍 결정용
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	int32 SpawnRelevancyThreshold = 0;
	
	// 스폰 수량을 데이터테이블 대신 고정값으로 쓸지용
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	bool bUseFixedSpawnQuantity = false;

	// bUseFixedSpawnQuantity가 true일 때 사용할 고정 스폰 수량
	// 메타값은 최소값 clamp, bool 변수 값이 false이면 안보이도록 하는 역할
	UPROPERTY(EditAnywhere, Category = "SpawnerSet",
		meta = (ClampMin = "0", EditCondition = "bUseFixedSpawnQuantity"))
	int32 FixedSpawnQuantity = 0;

private:
	TSoftClassPtr<ANSEnemyCharacterBase> SoftCharacterClass;
	TSoftObjectPtr<UNSEnemyData> SoftEnemyData;
	int32 FinalSpawnQuantity = 0;
	TSharedPtr<FStreamableHandle> StreamingHandle;
	
	UPROPERTY(Transient)
	TObjectPtr<URoom> CachedRoom;
};
