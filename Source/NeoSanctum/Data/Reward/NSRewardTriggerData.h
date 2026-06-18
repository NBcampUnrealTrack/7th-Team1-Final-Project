// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSRewardTypes.h"
#include "NSRewardTriggerData.generated.h"

class UDataTable;

/**
 * 보상 트리거별 보상 데이터
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSRewardTriggerData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	FGameplayTag TriggerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward")
	TArray<FNSRewardEntry> RewardEntries;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Reward", meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> DropTable;
};
