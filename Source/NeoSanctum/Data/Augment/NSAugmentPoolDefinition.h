// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSAugmentTypes.h"
#include "NSAugmentPoolDefinition.generated.h"

class UNSAugmentDefinition;

// 카드 뽑기 덱(어떤 카드들이 들어있고, 각 등급의 확률이 어떤지)
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSAugmentPoolDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// Pool 식별 -> 일반 or 고등급 풀
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta")
	FGameplayTag PoolTag;

	// Rarity별 추첨 가중치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weights")
	TMap<ENSAugmentRarity, float> RarityWeights;
	
	/**
	 * 이 풀의 증강 후보 목록
	 * 일반풀: Common/Rare/Epic 만 등록, 고등급풀: Common/Rare/Epic + Legendary(기믹 변경) 등록
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entries")
	TArray<TSoftObjectPtr<UNSAugmentDefinition>> Entries;
	
	/**
	 * 지금이 고등급 풀이고 플레이어의 Legendary증강이 최대개수에 도달한경우 수치강화 Legendary증강이 들어감
	 * 다른 Common/Rare/Epic처럼 StackEffectClass만 설정된 Legendary Definition을 등록 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entries")
	TArray<TSoftObjectPtr<UNSAugmentDefinition>> LegendaryStatEntries;
};
