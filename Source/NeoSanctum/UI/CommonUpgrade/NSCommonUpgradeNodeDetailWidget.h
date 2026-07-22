// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NSCommonUpgradeNodeDetailWidget.generated.h"

class UTextBlock;
/**
 * 공용 업그레이드 노드 호버 시 뜨는 공용 디테일 패널.
 * 표시 전용이며, 구매 클릭은 그리드 카드(UNSCommonUpgradeNodeWidget)가 담당.
 */
UCLASS()
class NEOSANCTUM_API UNSCommonUpgradeNodeDetailWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// NewLevel/NextCost는 상위 위젯이 계산해서 표시용으로 전달(구매 호출과는 무관).
	void SetupDetail(const FNSCommonUpgradeNodeRow& Row, int32 CurrentLevel, int32 NewLevel, int64 NextCost);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CategoryText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CurrentGradeContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentGradeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GradeDividerSizeBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NextGradeContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextGradeText;

	// 비용 구분선과 비용 행을 함께 제어.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PurchaseSectionContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PurchaseLabelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PurchaseCostText;

private:
	static FText FormatModifierValue(float Value, ENSCombatStatModifierOperation Operation);
};
