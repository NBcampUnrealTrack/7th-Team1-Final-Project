// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSSkillUIData.generated.h"

class UTexture2D;

/**
 *  스킬 슬롯에 표시할 데이터
 */
USTRUCT(BlueprintType)
struct FNSSkillUIData : public FTableRowBase
{
	GENERATED_BODY()
	
	//스킬 식별 및 GMS메시지 필터에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FGameplayTag SkillTag;
	//UI에 표시할 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FText DisplayName;
	//스킬 슬롯 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Skill")
	TSoftObjectPtr<UTexture2D> SkillIcon;
};
