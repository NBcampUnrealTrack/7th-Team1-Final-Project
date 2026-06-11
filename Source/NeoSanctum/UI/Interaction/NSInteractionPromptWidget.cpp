// Copyright 2026 One Team. All rights reserved.


#include "NSInteractionPromptWidget.h"
#include "Components/TextBlock.h"

void UNSInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UNSInteractionPromptWidget::SetPromptText(
	const FText& InKeyText,
	const FText& InActionText)
{
	if (KeyText)
	{
		KeyText->SetText(InKeyText);
	}
	
	if (ActionText)
	{
		ActionText->SetText(InActionText);
	}
}
