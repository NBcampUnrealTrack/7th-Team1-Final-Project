// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentCardWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"
#include "Animation/WidgetAnimation.h"

void UNSAugmentCardWidget::SetAugmentName(const FString& NewName)
{
	EnsureCardContentVisible();

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
	EnsureCardContentVisible();

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
	EnsureCardContentVisible();

	if (!AugmentIcon)
	{
		return;
	}

	if (!NewIcon)
	{
		AugmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	AugmentIcon->SetBrushFromTexture(NewIcon, false);
	AugmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNSAugmentCardWidget::SetShortcutNumber(int32 NewShortcutNumber)
{
	static_cast<void>(NewShortcutNumber);
}

void UNSAugmentCardWidget::ApplyViewData(const FNSAugmentCardViewData& ViewData)
{
	EnsureCardContentVisible();

	CurrentRarity = ViewData.Rarity;

	// ViewData 희귀도에 맞춰 선택 텍스처를 미리 갱신
	RefreshSelectedCardVisual();

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

void UNSAugmentCardWidget::PlaySelectAnimation()
{
	// 애니메이션 시작 전에 카드 내부 요소를 보이게 함
	EnsureCardContentVisible();

	// 선택된 카드는 흐림 효과가 적용되지 않도록 함
	SetRenderOpacity(1.f);

	if (CardSelectedBackground)
	{
		// 선택 직전에 현재 희귀도에 맞는 선택 텍스처를 적용
		RefreshSelectedCardVisual();

		CardSelectedBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
		CardSelectedBackground->SetRenderOpacity(0.f);
	}

	// 선택 애니메이션이 있으면 재생
	if (Anim_SelectCard)
	{
		PlayAnimation(Anim_SelectCard, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}
	else if (CardSelectedBackground)
	{
		// 애니메이션이 없으면 선택 텍스처만 즉시 표시
		CardSelectedBackground->SetRenderOpacity(1.f);
	}
}

void UNSAugmentCardWidget::ResetSelectionVisual()
{
	// 선택 애니메이션이 재생 중이면 중지
	if (Anim_SelectCard)
	{
		StopAnimation(Anim_SelectCard);
	}

	// 카드 전체 투명도를 기본값으로 되돌림
	SetRenderOpacity(1.f);

	// 카드 이동 연출을 기본 위치로 되돌림
	if (CardSizeBox)
	{
		CardSizeBox->SetRenderTranslation(FVector2D::ZeroVector);
	}

	// 기본 카드 배경을 다시 보이게 함
	if (CardBackground)
	{
		CardBackground->SetRenderOpacity(1.f);
	}

	// 선택 텍스처 레이어를 숨김
	if (CardSelectedBackground)
	{
		CardSelectedBackground->SetRenderOpacity(0.f);
		CardSelectedBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 카드 내부 요소의 가시성을 보정
	EnsureCardContentVisible();
}

void UNSAugmentCardWidget::SetDeselectedVisual(float InOpacity)
{
	// 남아 있는 선택 애니메이션을 중지
	if (Anim_SelectCard)
	{
		StopAnimation(Anim_SelectCard);
	}

	// 선택되지 않은 카드에서는 선택 텍스처를 숨김
	if (CardSelectedBackground)
	{
		CardSelectedBackground->SetRenderOpacity(0.f);
	}

	// 선택되지 않은 카드의 전체 투명도를 낮춤
	SetRenderOpacity(FMath::Clamp(InOpacity, 0.f, 1.f));

	// 선택되지 않은 카드는 원래 위치에 고정
	if (CardSizeBox)
	{
		CardSizeBox->SetRenderTranslation(FVector2D::ZeroVector);
	}
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
		// 카드 슬롯/SizeBox가 330x118 크기를 담당하므로 텍스처 원본 크기로 Brush Size를 맞추지 않음
		CardBackground->SetBrushFromTexture(Texture, false);
	}

	// 텍스처 프로퍼티가 비어 있어도 WBP 기본 Brush를 유지할 수 있게 Collapsed 처리하지 않음
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

void UNSAugmentCardWidget::RefreshSelectedCardVisual()
{
	// 선택 배경 위젯이 없으면 선택 텍스처를 갱신하지 않음
	if (!CardSelectedBackground)
	{
		return;
	}

	UTexture2D* Texture = GetSelectedCardTextureForCurrentRarity();
	if (!Texture)
	{
		return;
	}

	// 선택 텍스처도 카드 슬롯 크기를 유지하도록 브러시 크기를 바꾸지 않음
	CardSelectedBackground->SetBrushFromTexture(Texture, false);
}

UTexture2D* UNSAugmentCardWidget::GetSelectedCardTextureForCurrentRarity() const
{
	switch (CurrentRarity)
	{
	case ENSAugmentRarity::Rare:
		return RareSelectedCardTexture
			       ? RareSelectedCardTexture.Get()
			       : CommonSelectedCardTexture.Get();

	case ENSAugmentRarity::Epic:
		return EpicSelectedCardTexture
			       ? EpicSelectedCardTexture.Get()
			       : CommonSelectedCardTexture.Get();

	case ENSAugmentRarity::Legendary:
		return LegendarySelectedCardTexture
			       ? LegendarySelectedCardTexture.Get()
			       : CommonSelectedCardTexture.Get();

	case ENSAugmentRarity::Common:
	default:
		return CommonSelectedCardTexture.Get();
	}
}

void UNSAugmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 카드 기본 상태에서는 강조를 끔
	bIsHighlighted = false;

	// 카드 선택 연출 상태를 초기화
	ResetSelectionVisual();

	// 현재 희귀도와 강조 상태에 맞는 배경을 적용
	RefreshCardVisual();
}
