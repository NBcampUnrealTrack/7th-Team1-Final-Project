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
	
	// 이 풀이 보유한 모든 증강 후보, Legendary 등급은 기믹 변경 후보를 여기에 등록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entries")
	TArray<TSoftObjectPtr<UNSAugmentDefinition>> Entries;

	/**
	 * Legendary의 최대 개수를 채우면 증강 뽑기시 Legendary자리에 들어갈 증강들
	 * Common,Rare,Epic과 비슷하게 수치증가만 있고 Legendary라 큰 증가폭을 가짐
	 * 색깔이나 이펙트는 Legendary랑 같은싱그로
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entries")
	TArray<TSoftObjectPtr<UNSAugmentDefinition>> LegendaryStatEntries;
};
