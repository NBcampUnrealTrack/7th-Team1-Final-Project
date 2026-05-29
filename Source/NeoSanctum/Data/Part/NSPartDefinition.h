// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartDefinition.generated.h"

class UGameplayEffect;
class UGameplayAbility;

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSPartDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	ENSPartSlot PartSlot = ENSPartSlot::Body;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	FText PartName;

	// 레그 파츠는 false —> 인런 밸런스상 리롤 불가
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	bool bCanReroll = true;

	// 등급별 수치 범위 (min~max), 장착/리롤/등급업 시 이 범위에서 랜덤 결정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	TMap<ENSPartRarity, FNSPartValueRange> ValueRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|GAS", meta = (AssetBundles = "InRunData"))
	TSoftClassPtr<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|Visual", meta = (AssetBundles = "OutRunUI,InRunUI"))
	TSoftObjectPtr<UTexture2D> Icon;

	// 번들 태그 없음 —> 드랍 발생 시 몬스터 사망 애니메이션 동안 온디맨드 로드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|Visual")
	TSoftObjectPtr<UStaticMesh> DropMesh;

	// 레전더리 등급 기믹 GA 확장 대비
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|GAS", meta = (AssetBundles = "InRunData"))
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;
};
