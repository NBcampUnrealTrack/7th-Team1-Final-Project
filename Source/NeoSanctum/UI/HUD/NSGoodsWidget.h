// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSGoodsWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * 인런 중 획득한 공용 재화와 임시 재화를 표시하는 HUD 위젯.
 */
UCLASS()
class NEOSANCTUM_API UNSGoodsWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//런 인 재화 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetRunInGoodsAmount(int32 NewGoodsAmount);
	//런 아웃 재화 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetRunOutGoodsAmount(int32 NewGoodsAmount);
	//런 인 재화 획득시 증가
	UFUNCTION(BlueprintCallable, Category = "UI")
	void AddRunInGoodsAmount(int32 AddAmount);
	//런 아웃 재화 획득시 증가
	UFUNCTION(BlueprintCallable, Category = "UI")
	void AddRunOutGoodsAmount(int32 AddAmount);
	//런 인 재화 사용
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UseRunInGoodsAmount(int32 UseAmount);
	//런 아웃 재화 사용
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UseRunOutGoodsAmount(int32 UseAmount);
	//런 시작시 초기화
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetRunInGoodsAmount();

private:
	//런 인에서 사용하는 휘발성재화
	int32 CurrentRunInGoodsAmount = 0;
	//런 아웃에서 사용하는 재화
	int32 CurrentRunOutGoodsAmount = 0;
	//런 인 재화 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> RunInGoodsText;
	//런 아웃 재화 텍스트
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RunOutGoodsText;
	
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> RunInGoodsIcon;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> RunOutGoodsIcon;

	
	//DataTable에서 재화 아이콘 적용
	void ApplyGoodsUIData();
	
protected:
	virtual void NativeConstruct() override;
};
