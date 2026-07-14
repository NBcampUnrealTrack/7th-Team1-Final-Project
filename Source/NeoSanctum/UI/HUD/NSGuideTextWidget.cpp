// Copyright 2026 One Team. All rights reserved.

#include "NSGuideTextWidget.h"
#include "CommonTextBlock.h"

void UNSGuideTextWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 안내 로직이 켜기 전까지 숨김
	HideGuideText();
}

void UNSGuideTextWidget::ShowGuideText(const FText& InText)
{
	// WBP에 GuideText가 배치되지 않았으면 안내 기능 없이 동작
	if (!GuideText)
	{
		return;
	}

	GuideText->SetText(InText);
	GuideText->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 배경도 텍스트와 함께 표시 (WBP에 배치 안 했으면 무시)
	if (GuideTextBackground)
	{
		GuideTextBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSGuideTextWidget::HideGuideText()
{
	if (!GuideText)
	{
		return;
	}

	GuideText->SetVisibility(ESlateVisibility::Collapsed);

	if (GuideTextBackground)
	{
		GuideTextBackground->SetVisibility(ESlateVisibility::Collapsed);
	}
}
