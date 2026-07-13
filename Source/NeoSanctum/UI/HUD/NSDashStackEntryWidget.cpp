// Copyright 2026 One Team. All rights reserved.


#include "NSDashStackEntryWidget.h"
#include "Components/Image.h"

void UNSDashStackEntryWidget::SetActive(bool bActive)
{
	if (ActiveImage)
	{
		ActiveImage->SetVisibility(
			bActive
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (InactiveImage)
	{
		InactiveImage->SetVisibility(
			bActive
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
}

