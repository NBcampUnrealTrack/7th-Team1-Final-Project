// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSSkillUIData.generated.h"

class UTexture2D;

/**
 * 캐릭터 선택 화면에서 스킬 수치를 어떤 형태로 보여줄지 정함.
 */
UENUM(BlueprintType)
enum class ENSSkillStatValueFormat : uint8
{
	Number UMETA(DisplayName = "숫자"),
	Percentage UMETA(DisplayName = "퍼센트"),
	Seconds UMETA(DisplayName = "초")
};

/**
 * 스킬 하나에서 캐릭터 선택 화면에 노출할 스탯 하나를 정의.
 */
USTRUCT(BlueprintType)
struct FNSSkillStatDisplayData
{
	GENERATED_BODY()

	// DT_AbilityBaseStats에서 값을 찾을 때 사용할 스탯 태그.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|CharacterSelect", meta = (Categories = "CombatStat"))
	FGameplayTag StatTag;

	// 태그 이름 대신 UI에 직접 보여줄 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|CharacterSelect")
	FText DisplayName;

	// 숫자 뒤에 %, 초 같은 단위를 붙일 때 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|CharacterSelect")
	ENSSkillStatValueFormat ValueFormat = ENSSkillStatValueFormat::Number;

	// 실제 저장값을 화면용 값으로 바꿀 때 곱함.
	// ex) 0.5를 50%로 보여주려면 100을 넣음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|CharacterSelect")
	float DisplayScale = 1.0f;
};

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

	// 캐릭터 선택 화면에 표시할 스킬 설명.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FText Description;

	// HUD 슬롯은 베젤이 포함된 기존 아이콘을 계속 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TSoftObjectPtr<UTexture2D> SkillIcon;

	// 캐릭터 선택 슬롯과 상세 패널에는 무프레임 아이콘을 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|CharacterSelect")
	TSoftObjectPtr<UTexture2D> CharacterSelectIcon;

	// 캐릭터 선택 화면에 보여줄 스탯만 순서대로 넣음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|CharacterSelect")
	TArray<FNSSkillStatDisplayData> CharacterSelectStats;
};
