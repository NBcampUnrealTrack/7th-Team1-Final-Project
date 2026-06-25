// Copyright 2026 One Team. All rights reserved.


#include "NSAmmoWidget.h"
#include "CommonTextBlock.h"

void UNSAmmoWidget::SetAmmo(int32 CurrentAmmo, int32 MaxAmmo)
{
	if (!AmmoText)
	{
		return;
	}
	
	AmmoText->SetText(FText::Format(
		NSLOCTEXT("AmmoWidget", "AmmoFormat", "{0} / {1}"),
		FText::AsNumber(FMath::Max(CurrentAmmo, 0)),
		FText::AsNumber(FMath::Max(MaxAmmo, 0))));
}

void UNSAmmoWidget::SetReloading(bool bReloading)
{
	bIsReloading = bReloading;

	RefreshAmmoText();
}

void UNSAmmoWidget::RefreshAmmoText()
{
	if (!AmmoText)
	{
		return;
	}

	if (bIsReloading)
	{
		AmmoText->SetText(NSLOCTEXT("AmmoWidget", "ReloadingText", "Reload"));
		return;
	}

	AmmoText->SetText(FText::Format(
		NSLOCTEXT("AmmoWidget", "AmmoFormat", "{0} / {1}"),
		FText::AsNumber(CachedCurrentAmmo),
		FText::AsNumber(CachedMaxAmmo)));
}
