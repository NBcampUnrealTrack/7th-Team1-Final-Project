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

	if (IsValid(PurchaseSectionContainer))
	{
		// 최대 등급이면 비용 구분선과 비용 행을 모두 숨김.
		PurchaseSectionContainer->SetVisibility(
			bMaxLevel ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	// 레벨 0(아직 안 산 노드)이면 "현재 등급" 영역은 보여줄 내용이 없으므로 숨김.
	if (IsValid(CurrentGradeContainer))
	{
		CurrentGradeContainer->SetVisibility(
			CurrentLevel > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(GradeDividerSizeBox))
	{
		// 현재 등급과 다음 등급이 모두 있을 때만 구분선을 보여줌.
		const bool bShowGradeDivider = CurrentLevel > 0 && !bMaxLevel;

		GradeDividerSizeBox->SetVisibility(
			bShowGradeDivider ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CurrentLevel > 0 && IsValid(CurrentGradeText))
	{
		// 이름은 위쪽 제목에서 보여주니까 여기서는 현재 값만 보여줌.
		CurrentGradeText->SetText(FormatModifierValue(Row.ValuePerLevel * CurrentLevel, Row.Operation));
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
			// 다음 등급도 같은 형식으로 효과 값만 보여줌.
			NextGradeText->SetText(FormatModifierValue(Row.ValuePerLevel * NewLevel, Row.Operation));
		}

		if (IsValid(PurchaseLabelText))
		{
			// 비용 영역은 구매할 등급 대신 역할을 바로 알 수 있게 표시.
			PurchaseLabelText->SetText(NSLOCTEXT("CommonUpgrade", "PurchaseCostLabel", "업그레이드 비용"));
		}

		if (IsValid(PurchaseCostText))
		{
			PurchaseCostText->SetText(FText::AsNumber(NextCost));
		}
	}
}

FText UNSCommonUpgradeNodeDetailWidget::FormatModifierValue(float Value, ENSCombatStatModifierOperation Operation)
{
	const FText NumberText = FText::AsNumber(Value);

	// 양수에는 +를 붙이고, 음수의 -는 AsNumber 결과를 그대로 사용.
	const FText SignedValueText = Value > 0
		? FText::Format(
			NSLOCTEXT("CommonUpgrade", "PositiveModifierValue", "+{0}"), NumberText) : NumberText;

	return Operation == ENSCombatStatModifierOperation::Add
		? SignedValueText
		: FText::Format(NSLOCTEXT("CommonUpgrade", "ModifierValueMultiply", "{0}%"), SignedValueText);
}
