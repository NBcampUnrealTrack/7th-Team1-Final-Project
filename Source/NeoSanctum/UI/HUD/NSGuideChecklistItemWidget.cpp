// Copyright 2026 One Team. All rights reserved.

#include "NSGuideChecklistItemWidget.h"
#include "CommonTextBlock.h"
#include "Animation/WidgetAnimation.h"

void UNSGuideChecklistItemWidget::Setup(FName InItemId, const FText& InText)
{
	ItemId = InItemId;

	// 문구 바인딩이 없으면 표시할 수 없으니 조용히 무시
	if (!ItemText)
	{
		return;
	}
	ItemText->SetText(InText);
}

void UNSGuideChecklistItemWidget::PlayCompleteAnimation()
{
	/**
	 * WBP에 완료 애니메이션이 없으면 애니메이션 없이 즉시 완료로 간주하고 통보한다
	 * (컨테이너가 이 줄을 바로 제거)
	 */
	if (!CompleteAnimation)
	{
		OnItemCompleteFinished.Broadcast(this);
		return;
	}

	// 애니메이션 종료 시점에 완료를 통보하도록 바인딩 후 재생
	FWidgetAnimationDynamicEvent FinishedEvent;
	FinishedEvent.BindDynamic(this, &UNSGuideChecklistItemWidget::HandleCompleteAnimationFinished);
	BindToAnimationFinished(CompleteAnimation, FinishedEvent);

	PlayAnimation(CompleteAnimation);
}

void UNSGuideChecklistItemWidget::HandleCompleteAnimationFinished()
{
	OnItemCompleteFinished.Broadcast(this);
}
