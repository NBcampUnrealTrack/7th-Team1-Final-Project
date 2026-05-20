// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSCrosshairWidget.generated.h"

class UImage;

/**
 * 플레이어의 조준점을 표시하는 HUD위젯
 */
UCLASS()
class NEOSANCTUM_API UNSCrosshairWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//조준점 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowCrosshair();
	//조준점 숨김
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideCrosshair();
	//조준점 색상 변경
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCrosshairColor(FLinearColor Newcolor);

private:
	//조준점 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairImage;
	
protected:
	virtual void NativeConstruct() override;
};
