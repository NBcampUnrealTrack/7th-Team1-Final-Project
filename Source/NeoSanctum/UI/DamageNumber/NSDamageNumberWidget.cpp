// Copyright 2026 One Team. All rights reserved.


#include "NSDamageNumberWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"

void UNSDamageNumberWidget::SetDamageNumber(const FNSDamageNumberFeedbackContext& Context)
{
	DisplayDamage = FMath::RoundToInt(Context.DamageAmount);
	bCritical = Context.bIsCritical;

	if (DamageText)
	{
		// 위로 올라가는 애니메이션과 겹치지 않게 시작 위치는 Canvas Slot에서 나눔.
		if (UCanvasPanelSlot* DamageTextSlot = Cast<UCanvasPanelSlot>(DamageText->Slot))
		{
			DamageTextSlot->SetPosition(DisplayOffset);
		}

		DamageText->SetText(FText::AsNumber(DisplayDamage));
		DamageText->SetColorAndOpacity(bCritical ? CriticalDamageColor : NormalDamageColor);
		DamageText->SetRenderScale(bCritical ? CriticalRenderScale : NormalRenderScale);
	}

	// WBP에 애니메이션이 있으면 숫자가 뜨는 연출만 재생.
	if (PopupAnimation)
	{
		PlayAnimation(PopupAnimation);
	}
}
