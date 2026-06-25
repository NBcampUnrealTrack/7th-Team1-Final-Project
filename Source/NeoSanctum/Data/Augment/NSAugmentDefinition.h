// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NSAugmentTypes.h"
#include "NSAugmentDefinition.generated.h"

class UTexture2D;
class UGameplayEffect;
class UGameplayAbility;

// 증강 카드 한장의 내용 (이름, 아이콘, 어떤 효과인지)
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSAugmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta")
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta")
	FGameplayTag AugmentTag;

	// 이 증강을 누적 선택 가능한 최대 스택 수 (도달 시 카드 후보에서 제외). 에디터에서 증강별 조정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meta", meta = (ClampMin = "1"))
	int32 MaxStack = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;
	
	/**
	 * Common / Rare / Epic / Legendary (수치 강화) 같은 스택형 GE저장
	 * SetByCaller로 현재 스택 카운터 전달
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (AssetBundles = "InRunData"))
	TSoftClassPtr<UGameplayEffect> StackEffectClass;
	
	/**
	 * 기믹 변경 GA저장 -> Legendary
	 * 가질수 있는 최대개수제한이 있음 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (AssetBundles = "InRunData"))
	TSoftClassPtr<UGameplayAbility> GrantedAbilityClass;
};
