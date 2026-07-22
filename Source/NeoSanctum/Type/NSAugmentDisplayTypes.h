// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/Augment/NSAugmentTypes.h"
#include "NSAugmentDisplayTypes.generated.h"

class UTexture2D;

/**
 * 증강 표시값 Bridge가 카드 UI에 전달하는 완성된 표시 데이터
 * 카드 위젯은 증강 시스템이나 DataTable을 직접 조회하지 않고 이 구조체만 사용
 */
USTRUCT(BlueprintType)
struct FNSAugmentCardViewData
{
	GENERATED_BODY()

	//증강 DataAsset을 식별하는 Primary Asset ID
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	FPrimaryAssetId DefId;

	//증강을 구분하는 Gameplay Tag
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	FGameplayTag AugmentTag;

	//현지화가 적용되는 증강 이름
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	FText DisplayName;

	//실제 수치가 삽입된 최종 현지화 설명
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	FText Description;

	//카드에 표시할 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	TSoftObjectPtr<UTexture2D> Icon;

	//카드의 희귀도
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;

	//현재 보유 중첩 수
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	int32 CurrentStack = 0;

	// 최대 중첩 수
	UPROPERTY(BlueprintReadOnly, Category = "UI|Augment")
	int32 MaxStack = 1;
};