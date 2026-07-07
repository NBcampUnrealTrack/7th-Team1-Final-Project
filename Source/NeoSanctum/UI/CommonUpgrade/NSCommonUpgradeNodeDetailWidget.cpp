// Copyright 2026 One Team. All rights reserved.


#include "NSCommonUpgradeNodeDetailWidget.h"

#include "Components/TextBlock.h"

void UNSCommonUpgradeNodeDetailWidget::SetupDetail(const FNSCommonUpgradeNodeRow& Row, int32 CurrentLevel,
                                                   int32 NewLevel, int64 NextCost)
{
	if (IsValid(CategoryText))
	{
		CategoryText->SetText(FText::Format(
			NSLOCTEXT("CommonUpgrade", "CategoryFormat", "// {0}"),
			UEnum::GetDisplayValueAsText(Row.Category))
		);
	}

	if (IsValid(TitleText))
	{
		TitleText->SetText(Row.DisplayName);
	}

	if (IsValid(DescriptionText))
	{
		DescriptionText->SetText(Row.Description);
	}

	const bool bMaxLevel = CurrentLevel >= Row.MaxLevel;

	// 레벨 0(아직 안 산 노드)이면 "현재 등급" 영역은 보여줄 내용이 없으므로 숨김.
	if (IsValid(CurrentGradeContainer))
	{
		CurrentGradeContainer->SetVisibility(
			CurrentLevel > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CurrentLevel > 0 && IsValid(CurrentGradeText))
	{
		// 최고 등급이면 카드 그리드에도 이미 "MaxLevel/MaxLevel"이 보이므로 숫자 대신 상태 문구로 대체.
		const FText LevelLabel = bMaxLevel
			? NSLOCTEXT("CommonUpgrade", "MaxGradeLabel", "최고 등급")
			: FText::AsNumber(CurrentLevel);

		CurrentGradeText->SetText(FText::Format(
			NSLOCTEXT("CommonUpgrade", "CurrentGradeFormat", "{0} {1} {2}"),
			LevelLabel,
			Row.DisplayName,
			FormatModifierValue(Row.ValuePerLevel * CurrentLevel, Row.Operation))
		);
	}

	// 최대 레벨이면 더 살 수 있는 다음 등급/구매 안내가 없으므로 숨김.
	if (IsValid(NextGradeContainer))
	{
		NextGradeContainer->SetVisibility(
			bMaxLevel ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (!bMaxLevel)
	{
		if (IsValid(NextGradeText))
		{
			NextGradeText->SetText(FText::Format(
				NSLOCTEXT("CommonUpgrade", "NextGradeFormat", "{0} {1}"),
				Row.DisplayName,
				FormatModifierValue(Row.ValuePerLevel * NewLevel, Row.Operation))
			);
		}

		if (IsValid(PurchaseLabelText))
		{
			PurchaseLabelText->SetText(FText::Format(
				NSLOCTEXT("CommonUpgrade", "PurchaseLabelFormat", "{0} 등급 해금"),
				FText::AsNumber(NewLevel))
			);
		}

		if (IsValid(PurchaseCostText))
		{
			PurchaseCostText->SetText(FText::AsNumber(NextCost));
		}
	}
}

FText UNSCommonUpgradeNodeDetailWidget::FormatModifierValue(float Value, ENSCombatStatModifierOperation Operation)
{
	// 증가/감소 방향은 Row.DisplayName이 이미 설명하므로(예: "~증가", "~할인", "~감소")
	// 여기서는 절대값만 보여줘서 음수 값이 "+-0.5+"처럼 깨지는 걸 막음.
	const float AbsValue = FMath::Abs(Value);

	return Operation == ENSCombatStatModifierOperation::Add
		? FText::Format(NSLOCTEXT("CommonUpgrade", "ModifierValueAdd", "{0}"), FText::AsNumber(AbsValue))
		: FText::Format(NSLOCTEXT("CommonUpgrade", "ModifierValueMultiply", "{0}%"), FText::AsNumber(AbsValue));
}
