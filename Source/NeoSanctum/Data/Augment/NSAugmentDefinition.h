// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSAugmentDefinition.generated.h"

class UTexture2D;
class UGameplayEffect;
class UGameplayAbility;

/**
 * 증강 카드의 UI 정보와 실행 에셋을 제공하는 Primary Data Asset.
 *
 * 선택 규칙과 CombatStat Modifier 값은 DT_AugmentDefinition에서 관리.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSAugmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;
	
	// StackEffectClass에 ValuePerStack * Stacks 값을 전달할 SetByCaller 태그.
	// 비워두면 StackEffectClass에는 증강 수치 payload를 전달하지 않습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Augment|Effect",
		meta = (Categories = "Effect"))
	FGameplayTag StackEffectSetByCallerTag;
	
	/**
	 * Common / Rare / Epic / Legendary 같은 수치 강화 GE.
	 * StackEffectSetByCallerTag가 유효하면 SetByCaller로 ValuePerStack * Stacks 값을 전달합니다.
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
