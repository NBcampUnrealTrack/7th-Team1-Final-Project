// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSAugmentDefinition.generated.h"

class UTexture2D;
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

	/**
	 * 기믹 변경 GA저장 -> Legendary
	 * 가질수 있는 최대개수제한이 있음 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect", meta = (AssetBundles = "InRunData"))
	TSoftClassPtr<UGameplayAbility> GrantedAbilityClass;
};
