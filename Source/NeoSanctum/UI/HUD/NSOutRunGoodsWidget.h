// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSOutRunGoodsWidget.generated.h"

class UImage;
class UCommonTextBlock;

/**
 * 아웃런에서 보유 중인 공용 재화를 표시하는 HUD 위젯.
 */
UCLASS()
class NEOSANCTUM_API UNSOutRunGoodsWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	// 아웃런에서는 저장된 공용 재화와 방금 인런에서 획득한 공용 재화가 반영된 최종 보유량을 표시.
	UFUNCTION(BlueprintCallable, Category = "UI|Goods")
	void RefreshGoods();

	//영구재화 표시
	UFUNCTION(BlueprintCallable, Category = "UI|Goods")
	void SetCommonGoodsAmount(int32 NewAmount);

private:
	//아웃런 영구재화 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonGoodsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CommonGoodsIcon;

	// CommonDataConfig로 선로딩된 재화 UI 데이터를 위젯에 적용.
	void ApplyGoodsUIData();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void HandleCurrencyChanged(int64 CommonCurrency);
};
