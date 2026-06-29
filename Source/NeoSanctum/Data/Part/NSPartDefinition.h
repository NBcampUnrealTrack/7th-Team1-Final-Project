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
	FText PartName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|GAS", meta = (AssetBundles = "InRunData"))
	TSoftClassPtr<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|Visual", meta = (AssetBundles = "OutRunUI,InRunUI"))
	TSoftObjectPtr<UTexture2D> Icon;

	// 파츠 외형 메시 — 드랍 액터 표시 + 캐릭터 장착 비주얼 겸용. 번들 태그 없음(온디맨드 로드)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|Visual")
	TSoftObjectPtr<USkeletalMesh> PartMesh;

	// 레전더리 등급 기믹 GA 확장 대비
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part|GAS", meta = (AssetBundles = "InRunData"))
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;
};
