// Copyright 2026 One Team. All rights reserved.


#include "NSDamageNumberWidget.h"

#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"

void UNSDamageNumberWidget::SetDamageNumber(const FNSDamageNumberFeedbackContext& Context)
{
	DisplayDamage = FMath::RoundToInt(Context.DamageAmount);
	bCritical = Context.bIsCritical;
}
