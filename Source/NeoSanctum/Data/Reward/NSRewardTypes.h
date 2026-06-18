// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "NSRewardTypes.generated.h"

class UNSPartDefinition;
class UNSAugmentPoolDefinition;

/**
 * 보상 트리거에서 사용할 보상 항목 데이터
 */
USTRUCT(BlueprintType)
struct FNSRewardEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag RewardTypeTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	TSoftObjectPtr<UNSAugmentPoolDefinition> AugmentPool;
};

/**
 * 보상 드랍 테이블 행 데이터
 */
USTRUCT(BlueprintType)
struct FNSRewardDropRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag DropGroupTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag RewardTypeTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0"))
	int32 MinQuantity = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0"))
	int32 MaxQuantity = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag CurrencyTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	TSoftObjectPtr<UNSPartDefinition> PartDefinition;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag AugmentPoolTag;
};
