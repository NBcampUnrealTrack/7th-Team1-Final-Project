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
		CurrentGradeText->SetText(FText::Format(
			NSLOCTEXT("CommonUpgrade", "CurrentGradeFormat", "{0} {1} {2}"),
			FText::AsNumber(CurrentLevel),
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
	return Operation == ENSCombatStatModifierOperation::Add
		? FText::Format(NSLOCTEXT("CommonUpgrade", "ModifierValueAdd", "+{0}"), FText::AsNumber(Value))
		: FText::Format(NSLOCTEXT("CommonUpgrade", "ModifierValueMultiply", "+{0}%"), FText::AsNumber(Value));
}
