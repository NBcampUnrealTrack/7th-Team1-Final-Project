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

// 로딩 때 미리 풀링용
USTRUCT()
struct FNSPrewarmRequest
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UClass> CharacterClass = nullptr;
	UPROPERTY() TObjectPtr<UNSEnemyData> EnemyData = nullptr;
	int32 Count = 0;
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
	
	// 프리워밍 큐 세팅
	void PrewarmBegin(
		const TArray<FNSPrewarmRequest>& Requests,
		int32 PerTickCount,
		FSimpleDelegate OnComplete);
	// 한 틱 생성 아직 남았으면 true 끝났으면 false
	bool PrewarmStep();
	
	virtual UWorld* GetWorld() const override;


private:
	// 풀 키 = 데이터 에셋 (종류별 독립 풀)
	UPROPERTY(Transient)
	TMap<TObjectPtr<UNSEnemyData>, FNSMonsterPoolArray> PoolMap;

	// 프리워밍 진행 상태
	TArray<FNSPrewarmRequest> PrewarmQueue;
	// 현재 처리 중인 요청 인덱스
	int32 PrewarmQueueIndex = 0;   
	// 현재 요청에서 이미 만든 수
	int32 PrewarmMadeInCurrent = 0;
	int32 PrewarmPerTick = 8;
	FSimpleDelegate PrewarmOnComplete;

	// 캐시 미스 계측용
	int32 PoolMissCount = 0;
	
};
