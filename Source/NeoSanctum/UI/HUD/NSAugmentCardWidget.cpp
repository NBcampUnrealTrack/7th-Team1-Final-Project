// Copyright 2026 One Team. All rights reserved.


#include "NSAugmentCardWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "CommonTextBlock.h"

void UNSAugmentCardWidget::SetAugmentName(const FString& NewName)
{
	//증강 이름 텍스트 갱신
	if (!AugmentNameText)
	{
		return;
	}
	AugmentNameText->SetText(FText::FromString(NewName));
}

void UNSAugmentCardWidget::SetAugmentDescription(const FString& NewDescription)
{
	//증강 설명 텍스트 갱신
	if (!AugmentDescriptionText)
	{
		return;
	}
	AugmentDescriptionText->SetText(FText::FromString(NewDescription));
}

void UNSAugmentCardWidget::SetHighLighted(bool bHighLighted)
{
	//카드 선택 상태에 따라 테두리 색상 변경
	if (!CardBorder)
	{
		return;
	}
	
	const FLinearColor BorderColor = bHighLighted
	? GetRarityHighlightColor()
	: FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
	
	CardBorder->SetBrushColor(BorderColor);
}

void UNSAugmentCardWidget::SetAugmentIcon(UTexture2D* NewIcon)
{
	if (!AugmentIcon)
	{
		return;
	}
	if (!NewIcon)
	{
		AugmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	AugmentIcon->SetBrushFromTexture(NewIcon);
	AugmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::SetShortcutNumber(int32 NewShortcutNumber)
{
	if (!ShortcutNumberText)
	{
		return;
	}
	
	if (NewShortcutNumber <= 0)
	{
		ShortcutNumberText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	ShortcutNumberText->SetText(FText::AsNumber(NewShortcutNumber));
	ShortcutNumberText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::ApplyViewData(const FNSAugmentCardViewData& ViewData)
{
	CurrentRarity = ViewData.Rarity;
	SetHighLighted(true);
	
	if (AugmentNameText)
	{
		AugmentNameText->SetText(ViewData.DisplayName);
	}

	if (AugmentDescriptionText)
	{
		AugmentDescriptionText->SetText(ViewData.Description);
	}

	SetAugmentIcon(ViewData.Icon.Get());
}

FLinearColor UNSAugmentCardWidget::GetRarityHighlightColor() const
{
	switch (CurrentRarity)
	{
	case ENSAugmentRarity::Rare:
		return RareHighlightColor;
		
	case ENSAugmentRarity::Epic:
		return EpicHighlightColor;
		
	case ENSAugmentRarity::Legendary:
		return LegendaryHighlightColor;
		
	case ENSAugmentRarity::Common:
	default:
		return CommonHighlightColor;
	}
}

void UNSAugmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	
	
	//기본 상태는 하이라이트 비활성화
	SetHighLighted(false);
}
