// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSCharacterSkillUISet.generated.h"

class UTexture2D;

/**
 * 스킬 슬롯에 표시할 입력키 데이터
 */
USTRUCT(BlueprintType)
struct FNSInputDisplayData
{
	GENERATED_BODY()

	//입력 표시를 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bShowInputDisplay = true;
	
	//키보드 입력 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FText InputText;

	//마우스/패드 입력 이미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TSoftObjectPtr<UTexture2D> InputIcon;

	//텍스트 대신 이미지를 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bUseInputIcon = false;
};

/**
 * 캐릭터별 스킬 슬롯 UI를 구성하는 데이터
 */
USTRUCT(BlueprintType)
struct FNSCharacterSkillUISet : public FTableRowBase
{
	GENERATED_BODY()

	// 좌클릭 기본 공격에 표시할 스킬 UI 데이터.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle BaseAttackUIDataRow;

	//1번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill1UIDataRow;

	//2번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill2UIDataRow;

	//3번 슬롯에 표시할 스킬 UI 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FDataTableRowHandle Skill3UIDataRow;

	// 좌클릭 기본 공격의 입력 표시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FNSInputDisplayData BaseAttackInputDisplay;

	//1번 슬롯 입력 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FNSInputDisplayData Skill1InputDisplay;

	//2번 슬롯 입력 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FNSInputDisplayData Skill2InputDisplay;

	//3번 슬롯 입력 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FNSInputDisplayData Skill3InputDisplay;
};
