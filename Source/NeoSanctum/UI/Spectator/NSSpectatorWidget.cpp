// Copyright 2026 One Team. All rights reserved.


#include "NSSpectatorWidget.h"
#include "CommonTextBlock.h"

void UNSSpectatorWidget::SetSpectatingPlayerName(const FString& PlayerName)
{
	if (!SpectatingNameText)
	{
		return;
	}

	SpectatingNameText->SetText(FText::Format(
		NSLOCTEXT("SpectatorWidget", "SpectatingNameFormat", "관전자 : {0}"),
		FText::FromString(PlayerName)));
}
