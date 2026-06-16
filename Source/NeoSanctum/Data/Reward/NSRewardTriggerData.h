// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSRewardTypes.h"
#include "NSRewardTriggerData.generated.h"

/**
 * 몬스터 / 보상 트리거별 드랍 테이블 데이터
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSRewardTriggerData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag TriggerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	TArray<FNSRewardEntry> DropEntries;
};
