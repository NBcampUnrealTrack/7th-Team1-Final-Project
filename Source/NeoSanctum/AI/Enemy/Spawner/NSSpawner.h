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

/**
 * 스포너가 사용할 테이블의 출저.
 *
 * 기본 흐름은 LevelConfig의 스테이지 공용 DT를 사용하고,
 * Manual은 테스트/특수 스폰용 예외 경로.
 */
UENUM(BlueprintType)
enum class ENSSpawnDataTableSource : uint8
{
	StageMelee	UMETA(DisplayName = "Stage Melee"),
	StageRange	UMETA(DisplayName = "Stage Range"),
	Manual		UMETA(DisplayName = "Manual")
};

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
	FVector GetRandomSpawnLocation(const TArray<FVector>& AlreadyPlaced, float ZOffset) const;
	
	// 현재 스포너가 LevelConfig의 어떤 스테이지 공용 DT를 사용할지 결정.
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	ENSSpawnDataTableSource SpawnDataTableSource = ENSSpawnDataTableSource::Manual;
	
	// 인스턴스마다 어떤 몬스터 스폰할지 정하는 용도
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	TObjectPtr<UDataTable> SpawnDataTable;
	
	// 보스 스포너인지 알기 위한 게터 함수
	bool IsBossSpawner() const { return bIsBossSpawner; }
	
	// 재활성화 시 새로 뽑지 않고, 살아남은 수만큼만 복원
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	bool bPersistSurvivors = false;

private:
	void ProcessSpawnProbability(UDataTable* SpawnTable);
	void RequestAsyncLoad();
	void OnLoadCompleted();
	void ExecuteFinalSpawn();
	
	UDataTable* ResolveSpawnDataTable() const;
	void RequestStageSpawnerTableLoad();
	UFUNCTION()
	void HandleStageSpawnerTablesReady();
	
	bool bHasSpawned = false;
	
	// 스포너가 스폰한 몬스터 저장용
	UPROPERTY()
	TArray<TObjectPtr<ANSEnemyCharacterBase>> SpawnedMonsters;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// 이 스포너가 활성화되는 최소 접속 인원 (1=항상 켜짐, 2=2인 이상부터, 3=3인부터, 4=4인부터)
	UPROPERTY(EditAnywhere, Category = "SpawnerSet", meta = (ClampMin = "1"))
	int32 RequiredPlayerCount = 1;
	
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
	
	// 이 스포너가 보스 스포너인지 정하는 불 플래그
	UPROPERTY(EditAnywhere, Category = "SpawnerSet")
	bool bIsBossSpawner = false;

	// bUseFixedSpawnQuantity가 true일 때 사용할 고정 스폰 수량
	// 메타값은 최소값 clamp, bool 변수 값이 false이면 안보이도록 하는 역할
	UPROPERTY(EditAnywhere, Category = "SpawnerSet",
		meta = (ClampMin = "0", EditCondition = "bUseFixedSpawnQuantity"))
	int32 FixedSpawnQuantity = 0;

private:
	TSoftClassPtr<APawn> SoftCharacterClass;
	TSoftObjectPtr<UNSEnemyData> SoftEnemyData;
	int32 FinalSpawnQuantity = 0;
	TSharedPtr<FStreamableHandle> StreamingHandle;
	
	UPROPERTY(Transient)
	TObjectPtr<URoom> CachedRoom;
	
	// 복원 경로
	void RestoreSurvivors();

	// 최초 스폰을 한 번이라도 했는가
	bool bInitialSpawnDone = false;

	// 재활성화 시 복원할 살아남은 수
	int32 SurvivorCount = 0;

	// 최초 스폰에 쓴 클래스/데이터
	UPROPERTY()
	TObjectPtr<UClass> CachedCharacterClass;
	UPROPERTY()
	TObjectPtr<UNSEnemyData> CachedEnemyData;
	
	// 헬퍼용 함수: 몬스터를 지정 클래스/데이터로 스폰 (보스는 다른 로직 사용)
	void SpawnMonsters(UClass* InClass, UNSEnemyData* InData, int32 Count);
	// 헬퍼용 함수: 스폰 대상의 바닥 Z오프셋 계산용
	float ComputeSpawnZOffset(UClass* InClass, const UNSEnemyData* InData) const;
};
