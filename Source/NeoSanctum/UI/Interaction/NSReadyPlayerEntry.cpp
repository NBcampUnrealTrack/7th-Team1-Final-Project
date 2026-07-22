// Copyright 2026 One Team. All rights reserved.


#include "NSReadyPlayerEntry.h"
#include "Components/Image.h"
#include "CommonTextBlock.h"

void UNSReadyPlayerEntry::SetPlayer(
	UTexture2D* ClassIconTexture,
	const FString& PlayerName,
	const FString& ClassName,
	bool bIsReady)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 직업 아이콘
	if (ClassIconImage && ClassIconTexture)
	{
		ClassIconImage->SetBrushFromTexture(ClassIconTexture);
	}

	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerName));
	}
	if (ClassNameText) 
	{
		ClassNameText->SetText(FText::FromString(ClassName));
	}

	if (ReadyStatusText)
	{
		ReadyStatusText->SetText(
			bIsReady
				? NSLOCTEXT("ReadyEntry", "Ready", "준비 완료")
				: NSLOCTEXT("ReadyEntry", "Waiting", "준비 중"));
	}

	// 프레임
	if (FrameImage)
	{
		UObject* Frame = bIsReady ? FrameActiveSprite : FrameDefaultSprite;
		if (Frame)
		{
			FrameImage->SetBrushResourceObject(Frame);
		}
	}
}

void UNSReadyPlayerEntry::ClearSlot()
{
	SetVisibility(ESlateVisibility::Hidden);
}