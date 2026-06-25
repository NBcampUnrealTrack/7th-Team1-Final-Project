// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSAugmentTypes.h"
#include "NSAugmentRarityRuleSet.generated.h"

USTRUCT(BlueprintType)
struct FNSAugmentRarityRule
{
	GENERATED_BODY()
	
	// 증강 카드가 발생한 보상 트리거
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Augment|Rarity", 
		meta = (Categories = "Reward.Trigger"))
	FGameplayTag RewardTriggerTag;
	
	// 현재 오퍼 1회에 사용할 희귀도 추첨 가중치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Augment|Rarity")
	TMap<ENSAugmentRarity, int32> RarityWeights;
};

/**
 * 보상 트리거별 증강 희귀도 추첨 규칙을 정의하는 Data Asset.
 *
 * 증강 후보 목록은 DT_AugmentDefinition에서 구성하고,
 * 이 Data Asset은 보상 트리거별 희귀도 가중치만 관리한다.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSAugmentRarityRuleSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	/**
 	 * 보상 트리거별 희귀도 규칙 목록.
 	 * 같은 RewardTriggerTag는 RuleSet 안에서 하나만 존재해야 한다.
 	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Augment|Rarity",
		meta = (TitleProperty = "RewardTriggerTag"))
	TArray<FNSAugmentRarityRule> RarityRules;
};
