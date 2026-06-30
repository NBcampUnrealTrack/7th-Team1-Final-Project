// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSCompanionDefinition.generated.h"

class UGameplayEffect;
class UNSCompanionAbilitySet;

USTRUCT(BlueprintType)
struct FNSCompanionUpgradeNode
{
	GENERATED_BODY()
	
	// 태그로 노드 확인
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag NodeTag;
	
	// 노드 최대 레벨
	UPROPERTY(EditDefaultsOnly)
	int32 MaxLevel = 5;
	
	// 적용할 effect
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> UpgradeEffect;
	
	// SetByCaller로 넘길 태그
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag SetByCallerTag;
	
	// 레벨당 증가량
	UPROPERTY(EditDefaultsOnly)
	float MagnitudePerLevel = 0.f;
	
	//UI에 표시할 강화 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText DisplayName;
	
	//UI에 표시할 강화 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText Description;
	
	//노드 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> Icon;
	
	// 첫 강화 비용
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Upgrade|Cost",
		meta = (ClampMin = "0"))
	int64 BaseUpgradeCost = 0;

	// 레벨이 오를 때마다 추가되는 비용
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Upgrade|Cost",
		meta = (ClampMin = "0"))
	int64 CostIncreasePerLevel = 0;
};

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSCompanionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CompanionTag;
	
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> Icon;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<USkeletalMesh> CompanionMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNSCompanionAbilitySet> AbilitySet;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> TypeStatsEffect;
	
	//해금 전제조건 게이팅
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag RequiredCompanionTag;
	
	UPROPERTY(EditDefaultsOnly)
	int32 RequiredUpgradeCount = 5;
	
	// 이 드론이 가진 업그레이드 노드들
	UPROPERTY(EditDefaultsOnly)
	TArray<FNSCompanionUpgradeNode> UpgradeNodes;
};
