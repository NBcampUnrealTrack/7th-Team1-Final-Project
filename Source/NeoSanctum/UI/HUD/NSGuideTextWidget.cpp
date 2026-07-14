// Copyright 2026 One Team. All rights reserved.

#include "NSGuideTextWidget.h"
#include "CommonTextBlock.h"
#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"

void UNSGuideTextWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 안내 로직이 켜기 전까지 숨김
	HideGuideText();
	
	/**
	 * HUD는 트래블/리스폰마다 재생성(ClientRestart의 ClearHUD→CreateHUD)되므로
	 * 표시 중이던 안내를 유실하지 않도록 서브시스템에서 현재 상태를 당겨옴
	 */
	if (UWorld* World = GetWorld())
	{
		if (UNSOutRunGuideSubsystem* GuideSubsystem =
			World->GetSubsystem<UNSOutRunGuideSubsystem>())
		{
			GuideSubsystem->RefreshGuideForHUD();
		}
	}
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
