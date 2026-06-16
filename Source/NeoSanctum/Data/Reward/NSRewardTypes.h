// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "NSRewardTypes.generated.h"

class UNSAugmentPoolDefinition;

/**
 * 몬스터 / 보상 트리거에서 사용할 드랍 항목 데이터
 */
USTRUCT(BlueprintType)
struct FNSMonsterDropEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag RewardTypeTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	TSoftObjectPtr<UNSAugmentPoolDefinition> AugmentPool;																																
};
