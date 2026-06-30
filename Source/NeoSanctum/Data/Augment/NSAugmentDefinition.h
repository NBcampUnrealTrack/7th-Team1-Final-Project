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
 * StackEffectClass에 전달할 SetByCaller payload 매핑.
 *
 * DT_AugmentDefinition의 Row.StatTag와 SourceStatTag가 일치하는 Row를 찾아
 * Row의 Operation / ValuePerStack / Stacks 기준 계산값을 SetByCallerTag로 전달합니다.
 */
USTRUCT(BlueprintType)
struct FNSAugmentStackEffectSetByCallerMapping
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (Categories = "CombatStat"))
	FGameplayTag SourceStatTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (Categories = "Effect"))
	FGameplayTag SetByCallerTag;
};

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
	
	// StackEffectClass에 전달할 Row StatTag별 SetByCaller 매핑.
	// 비어 있거나 Row와 매칭되지 않으면 payload 없이 GE가 적용되며, 런타임 Warning을 남깁니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TArray<FNSAugmentStackEffectSetByCallerMapping> StackEffectSetByCallerMappings;
	
	/**
	 * Common / Rare / Epic / Legendary 같은 수치 강화 GE.
	 * StackEffectSetByCallerMappings에 따라 Row별 계산값을 SetByCaller로 전달합니다.
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
