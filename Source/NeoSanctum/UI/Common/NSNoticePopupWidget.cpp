// Copyright 2026 One Team. All rights reserved.

#include "NSNoticePopupWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UNSNoticePopupWidget::ShowBlocking(const FText& Message)
{
	// 진행 표시는 클릭/타이머로 닫히면 안 되므로 기존 자동 소멸 타이머를 제거한다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	ShowInternal(Message, ENSNoticeMode::Blocking);
}

void UNSNoticePopupWidget::ShowToast(const FText& Message, float Duration)
{
	ShowInternal(Message, ENSNoticeMode::Toast);

	// 재호출 시 타이머를 리셋해 마지막 토스트 기준으로 유지 시간을 계산한다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
		World->GetTimerManager().SetTimer(DismissTimerHandle, this, &UNSNoticePopupWidget::Dismiss, Duration, false);
	}
}

void UNSNoticePopupWidget::Dismiss()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	RemoveFromParent();
}

FReply UNSNoticePopupWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Blocking(저장 중 등)은 사용자가 임의로 닫을 수 없음
	if (Mode == ENSNoticeMode::Toast)
	{
		Dismiss();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UNSNoticePopupWidget::ShowInternal(const FText& Message, ENSNoticeMode InMode)
{
	Mode = InMode;

	if (IsValid(NoticeText))
	{
		NoticeText->SetText(Message);
	}

	// 다른 UI(상호작용 위젯 등)보다 위에 그려지도록 높은 ZOrder로 표시
	if (!IsInViewport())
	{
		AddToViewport(100);
	}
}
