// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentCardWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

void UNSAugmentCardWidget::SetAugmentName(const FString& NewName)
{
	// 증강 이름 텍스트 갱신
	if (!AugmentNameText)
	{
		return;
	}

	AugmentNameText->SetText(FText::FromString(NewName));
	AugmentNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::SetAugmentDescription(const FString& NewDescription)
{
	// 증강 설명 텍스트 갱신
	if (!AugmentDescriptionText)
	{
		return;
	}

	AugmentDescriptionText->SetText(FText::FromString(NewDescription));
	AugmentDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::SetHighLighted(bool bHighLighted)
{
	bIsHighlighted = bHighLighted;
	RefreshCardVisual();
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

	AugmentIcon->SetBrushFromTexture(NewIcon, true);
	AugmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::SetShortcutNumber(int32 NewShortcutNumber)
{
	static_cast<void>(NewShortcutNumber);
}

void UNSAugmentCardWidget::ApplyViewData(const FNSAugmentCardViewData& ViewData)
{
	CurrentRarity = ViewData.Rarity;

	if (AugmentNameText)
	{
		AugmentNameText->SetText(ViewData.DisplayName);
		AugmentNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (AugmentDescriptionText)
	{
		AugmentDescriptionText->SetText(ViewData.Description);
		AugmentDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetAugmentIcon(ViewData.Icon.Get());
	RefreshCardVisual();
}

void UNSAugmentCardWidget::RefreshCardVisual()
{
	if (!CardBackground)
	{
		return;
	}

	UTexture2D* Texture = GetCardTextureForCurrentState();
	if (Texture)
	{
		// 카드 슬롯/SizeBox가 330x118 크기를 담당하므로 텍스처 원본 크기로 Brush Size를 맞추지 않습니다.
		CardBackground->SetBrushFromTexture(Texture, false);
	}

	// 텍스처 프로퍼티가 비어 있어도 WBP 기본 Brush를 유지할 수 있게 Collapsed 처리하지 않습니다.
	CardBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
}

UTexture2D* UNSAugmentCardWidget::GetCardTextureForCurrentState() const
{
	switch (CurrentRarity)
	{
	case ENSAugmentRarity::Rare:
		return bIsHighlighted
			       ? RareHighlightedCardTexture.Get()
			       : RareCardTexture.Get();

	case ENSAugmentRarity::Epic:
		return bIsHighlighted
			       ? EpicHighlightedCardTexture.Get()
			       : EpicCardTexture.Get();

	case ENSAugmentRarity::Legendary:
		return bIsHighlighted
			       ? LegendaryHighlightedCardTexture.Get()
			       : LegendaryCardTexture.Get();

	case ENSAugmentRarity::Common:
	default:
		return bIsHighlighted
			       ? CommonHighlightedCardTexture.Get()
			       : CommonCardTexture.Get();
	}
}

void UNSAugmentCardWidget::EnsureCardContentVisible()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.f);

	if (CardSizeBox)
	{
		CardSizeBox->SetWidthOverride(CardDisplaySize.X);
		CardSizeBox->SetHeightOverride(CardDisplaySize.Y);
		CardSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		CardSizeBox->SetRenderOpacity(1.f);
	}

	if (CardContentHorizontalBox)
	{
		CardContentHorizontalBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		CardContentHorizontalBox->SetRenderOpacity(1.f);
	}

	if (AugmentNameText)
	{
		AugmentNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		AugmentNameText->SetRenderOpacity(1.f);
	}

	if (AugmentDescriptionText)
	{
		AugmentDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
		AugmentDescriptionText->SetRenderOpacity(1.f);
	}

	if (AugmentIcon)
	{
		AugmentIcon->SetRenderOpacity(1.f);
	}

	if (CardBackground)
	{
		CardBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
		CardBackground->SetRenderOpacity(1.f);
	}
}

void UNSAugmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 기본 상태에서는 강조를 끄고 일반 희귀도 텍스처를 사용
	bIsHighlighted = false;
	RefreshCardVisual();
}
