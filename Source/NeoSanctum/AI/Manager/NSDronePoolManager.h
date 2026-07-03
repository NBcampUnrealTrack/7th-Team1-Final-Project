// Copyright 2026 One Team. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSDronePoolManager.generated.h"

class UNSEnemyData;
class ANSEnemyDrone;

USTRUCT()
struct FNSDronePoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ANSEnemyDrone>> Drones;
};

UCLASS()
class NEOSANCTUM_API UNSDronePoolManager : public UObject
{
	GENERATED_BODY()

public:
	// 풀에서 드론을 꺼내거나(재사용) 없으면 신규 생성해 반환
	ANSEnemyDrone* GetPooledDrone(
		TSubclassOf<ANSEnemyDrone> DroneClass,
		UNSEnemyData* EnemyData,
		const FVector& Location,
		const FRotator& Rotation,
		const FNSDifficultyScale& Scale);

	// 드론을 비활성화해 풀로 반환
	void ReturnDroneToPool(ANSEnemyDrone* Drone);

private:
	// 풀 키 = 데이터 에셋 (종류별 독립 풀) — 기존 매니저와 동일
	UPROPERTY(Transient)
	TMap<TObjectPtr<UNSEnemyData>, FNSDronePoolArray> PoolMap;
};