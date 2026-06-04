// Copyright 2026 One Team. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSGoodsUIData.generated.h"

class UTexture2D;

/**
 *   HUD에 표시할 재화 데이터
 */
USTRUCT(BlueprintType)
struct FNSGoodsUIData : public FTableRowBase
{
	GENERATED_BODY()
		
	//재화종류 식별용 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goods")
	FGameplayTag GoodsTag;
	//UI에 표시할 재화 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goods")
	FText DisplayName;
	//재화 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goods")
	TSoftObjectPtr<UTexture2D> GoodsIcon;
};
