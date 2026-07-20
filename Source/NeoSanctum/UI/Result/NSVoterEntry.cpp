// Copyright 2026 One Team. All rights reserved.


#include "NSVoterEntry.h"
#include "Components/Image.h"
#include "CommonTextBlock.h"

void UNSVoterEntry::SetVoter(const FString& PlayerName)
{
	if (VoterNameText)
	{
		VoterNameText->SetText(FText::FromString(PlayerName));
	}
	if (VoterImage && FilledSprite)
	{
		VoterImage->SetBrushResourceObject(FilledSprite);
	}
}

void UNSVoterEntry::ClearVoter()
{
	if (VoterNameText)
	{
		VoterNameText->SetText(FText::GetEmpty());
	}
	if (VoterImage && EmptySprite)
	{
		VoterImage->SetBrushResourceObject(EmptySprite);
	}
}

void UNSVoterEntry::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ClearVoter();
}
