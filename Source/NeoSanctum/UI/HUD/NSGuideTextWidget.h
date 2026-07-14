// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSGuideTextWidget.generated.h"

class UCommonTextBlock;
class UWidget;

/**
 * 우측 상단 목표 안내 텍스트(배경 포함) 위젯 — 지속형, HideGuideText 호출까지 유지
 */
UCLASS()
class NEOSANCTUM_API UNSGuideTextWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 안내 텍스트 + 배경 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowGuideText(const FText& InText);
	// 안내 텍스트 + 배경 숨김
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideGuideText();

protected:
	virtual void NativeConstruct() override;

private:
	// 안내 문구
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> GuideText;

	// GuideText를 감싸는 배경(Border 등). 텍스트와 함께 표시/숨김 (없으면 텍스트만 토글)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GuideTextBackground;
};
