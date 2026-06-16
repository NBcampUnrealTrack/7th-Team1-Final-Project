// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "UObject/PrimaryAssetId.h"
#include "NSAugmentRewardPoolData.generated.h"

/**
 * 증강 보상 풀에서 사용할 후보 증강 데이터
 */
USTRUCT(BlueprintType)
struct FNSAugmentRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (AllowedTypes = "NSAugmentData"))
	FPrimaryAssetId AugmentDefId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSAugmentRewardPoolData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|AugmentReward")
	FGameplayTag PoolTag;
	
	// 후보 증강들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|AugmentReward")
	TArray<FNSAugmentRewardEntry> CandidateAugments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|AugmentReward", meta = (ClampMin = "1"))
	int32 SelectionCount = 3;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|AugmentReward", meta = (ClampMin = "0"))
	int32 RerollCost = 0;
};
