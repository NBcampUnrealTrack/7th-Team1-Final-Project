// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSOutRunGoodsWidget.generated.h"

class UTextBlock;
class UCommonTextBlock;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSOutRunGoodsWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//아웃런 재화 UI를 현재 보유량으로 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|Goods")
	void RefreshGoods();

	//영구재화 표시
	UFUNCTION(BlueprintCallable, Category = "UI|Goods")
	void SetCommonGoodsAmount(int32 NewAmount);

	//스킬재화 표시
	UFUNCTION(BlueprintCallable, Category = "UI|Goods")
	void SetSkillGoodsAmount(int32 NewAmount);
	
private:
	//아웃런 영구재화 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonGoodsText;

	//아웃런 스킬재화 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SkillGoodsText;
	
protected:
	virtual void NativeConstruct() override;
};
