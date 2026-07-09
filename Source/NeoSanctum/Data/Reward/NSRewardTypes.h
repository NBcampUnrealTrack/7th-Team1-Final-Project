// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "NSRewardTypes.generated.h"

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0"))
	int32 Weight = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0"))
	int32 MinQuantity = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0"))
	int32 MaxQuantity = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag CurrencyTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag AugmentPoolTag;

	// 회복 포션 식별 태그. Reward.Type.Heal 행에서만 사용하며, 이 태그로 DT_HealPotion을 조회.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag HealPotionTag;
	
	// 파츠 레어도 태그(Part.Rarity.*). Reward.Type.Part 행에서만 사용하고 전체 파츠 풀에서 랜덤 선택                                       
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (Categories = "Part.Rarity"))                                                                                            
	FGameplayTag RarityTag;
};

/**
 * 보상 드랍 테이블 판정 결과
 */
USTRUCT(BlueprintType)
struct FNSRewardDropResult
{
	GENERATED_BODY()
	
	// 어떤 드랍 그룹에서 나온 결과인지 구분
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag DropGroupTag;
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag RewardTypeTag;
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag CurrencyTag;
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag AugmentPoolTag;

	// 회복 포션 식별 태그 (Reward.Type.Heal 결과에서 사용)
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag HealPotionTag;
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag RarityTag;
};
