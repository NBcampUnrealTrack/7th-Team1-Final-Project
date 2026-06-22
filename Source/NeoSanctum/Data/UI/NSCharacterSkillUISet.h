// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSCharacterSkillUISet.generated.h"

/**
 * 캐릭터별 스킬 슬롯 UI를 구성하는	 데이터
 */
USTRUCT(BlueprintType)
struct FNSCharacterSkillUISet : public FTableRowBase
{
	GENERATED_BODY()

	//1번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill1UIDataRow;

	//2번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill2UIDataRow;

	//3번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill3UIDataRow;
};
