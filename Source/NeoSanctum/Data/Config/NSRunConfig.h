// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSRunConfig.generated.h"

class UNSAugmentRarityRuleSet;
class UNSDifficultyConfig;
class UDataTable;

/**
 * 인런 전체가 유지되는 동안 공통으로 사용하는 데이터 설정.
 * 
 * 증강 후보, 몬스터 기본 스탯, 난이도 계산 등 스테이지가 바뀌어도
 * 다시 로드할 필요가 없는 런 단위 데이터를 정의.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSRunConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	// 이번 런에서 사용할 증강 후보 목록. Definition DA 목록은 이 DT에서 수집.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Run|Augment",
		meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> AugmentDefinitionTable;

	// 이번 런에서 사용할 증강 희귀도/가중치 규칙.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Run|Augment",
		meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UNSAugmentRarityRuleSet> AugmentRarityRuleSet;

	// 몬스터의 기본 스탯 테이블. 난이도 배율 적용 전 기준값으로 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Run|Monster",
		meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UDataTable> MonsterAttributeTable;

	// 시간, 스테이지, 플레이어 수에 따른 몬스터 난이도 배율 설정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NS|Run|Difficulty",
		meta = (AssetBundles = "InRunData"))
	TSoftObjectPtr<UNSDifficultyConfig> DifficultyConfig;
};
