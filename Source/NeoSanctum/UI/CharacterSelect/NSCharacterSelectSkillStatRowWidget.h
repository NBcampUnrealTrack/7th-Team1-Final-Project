// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSCharacterSelectSkillStatRowWidget.generated.h"

class UCommonTextBlock;
class UWidget;

/**
 * 캐릭터 선택 화면에서 스킬 스탯 한 줄을 표시하는 위젯.
 *
 * 부모 위젯에서 전달받은 스탯 이름과 포맷된 값을 보여주고,
 * 마지막 행인지에 따라 아래쪽 구분선 표시 여부를 결정함.
 *
 * DataTable 조회나 숫자 단위 변환은 담당하지 않으며,
 * 화면에 전달받은 내용을 표시하는 역할만 맡음.
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterSelectSkillStatRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 부모 위젯에서 이미 계산한 이름과 값을 한 행에 적용.
	void SetStatData(const FText& InDisplayName, const FText& InValueText, bool bShowDivider);

protected:
	// 스탯 이름을 왼쪽에 표시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI|Skill")
	TObjectPtr<UCommonTextBlock> SkillStatNameText;

	// 포맷된 스탯 값을 오른쪽에 표시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI|Skill")
	TObjectPtr<UCommonTextBlock> SkillStatValueText;

	// 마지막 행이 아닐 때만 아래쪽 구분선을 표시.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI|Skill")
	TObjectPtr<UWidget> SkillStatDivider;
};
