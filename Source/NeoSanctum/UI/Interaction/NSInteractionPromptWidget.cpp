// Copyright 2026 One Team. All rights reserved.


#include "NSInteractionPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"

UNSInteractionPromptWidget::UNSInteractionPromptWidget()
{
	// 등급별 기본 색상
	CommonStyle.BackgroundColor    = FLinearColor(0.10f, 0.10f, 0.12f, 0.50f);
	CommonStyle.AccentColor        = FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);

	RareStyle.BackgroundColor      = FLinearColor(0.04f, 0.12f, 0.06f, 0.50f);
	RareStyle.AccentColor          = FLinearColor(0.10f, 0.80f, 0.20f, 1.0f);

	EpicStyle.BackgroundColor      = FLinearColor(0.10f, 0.04f, 0.14f, 0.50f);
	EpicStyle.AccentColor          = FLinearColor(0.60f, 0.10f, 0.90f, 1.0f);

	LegendaryStyle.BackgroundColor = FLinearColor(0.14f, 0.10f, 0.02f, 0.50f);
	LegendaryStyle.AccentColor     = FLinearColor(1.00f, 0.70f, 0.10f, 1.0f);
}

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

void UNSInteractionPromptWidget::SetPromptIcon(const TSoftObjectPtr<UTexture2D>& Icon)
{
	if (!PartIconImage)
	{
		return;
	}

	// 이전 로드 취소 (대상이 빠르게 바뀔 때 잘못된 아이콘 세팅 방지)
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (Icon.IsNull())
	{
		PartIconImage->SetBrushFromTexture(nullptr);
		PartIconImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 이미 로드되어 있으면 즉시 세팅
	if (UTexture2D* Loaded = Icon.Get())
	{
		PartIconImage->SetBrushFromTexture(Loaded);
		PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	const FSoftObjectPath IconPath = Icon.ToSoftObjectPath();
	IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(IconPath,
		FStreamableDelegate::CreateWeakLambda(this, [this, IconPath]()
		{
			UTexture2D* Loaded = Cast<UTexture2D>(IconPath.ResolveObject());
			if (Loaded && PartIconImage)
			{
				PartIconImage->SetBrushFromTexture(Loaded);
				PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}));
}

void UNSInteractionPromptWidget::SetPartName(const FText& InName)
{
	if (!PartNameText)
	{
		return;
	}
	PartNameText->SetText(InName);
}

void UNSInteractionPromptWidget::SetStatComparison(const FText& StatName, float OldValue, float NewValue, bool bHigherIsBetter)
{
	if (StatCompareText)
	{
		// 소수점은 버리고 정수로만 표시. 반올림이면 3.8이 4로 보여 실제보다 좋아 보이므로 내림 고정
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		Options.MinimumFractionalDigits = 0;
		Options.RoundingMode = ERoundingMode::ToNegativeInfinity;

		StatCompareText->SetText(FText::Format(
			NSLOCTEXT("InteractionPrompt", "StatCompareFormat", "{0} : {1} -> {2}"),
			StatName,
			FText::AsNumber(OldValue, &Options),
			FText::AsNumber(NewValue, &Options)));
		StatCompareText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 값이 그대로면 화살표 둘 다 숨김, 아니면 좋아짐/나빠짐에 해당하는 이미지만 표시
	const bool bChanged = !FMath::IsNearlyEqual(OldValue, NewValue);
	const bool bImproved = bChanged && (bHigherIsBetter ? (NewValue > OldValue) : (NewValue < OldValue));

	if (StatArrowUpImage)
	{
		StatArrowUpImage->SetVisibility(
			(bChanged && bImproved) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (StatArrowDownImage)
	{
		StatArrowDownImage->SetVisibility(
			(bChanged && !bImproved) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSInteractionPromptWidget::ClearStatComparison()
{
	if (StatCompareText)
	{
		StatCompareText->SetText(FText::GetEmpty());
		StatCompareText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (StatArrowUpImage)
	{
		StatArrowUpImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (StatArrowDownImage)
	{
		StatArrowDownImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSInteractionPromptWidget::SetRarityStyle(int32 RarityIndex)
{
	const FNSRarityPromptStyle& Style = GetRarityStyle(RarityIndex);

	if (RarityBackgroundBorder)
	{
		RarityBackgroundBorder->SetBrushColor(Style.BackgroundColor);
	}

	if (RarityAccentImage)
	{
		RarityAccentImage->SetColorAndOpacity(Style.AccentColor);
	}
}

const FNSRarityPromptStyle& UNSInteractionPromptWidget::GetRarityStyle(int32 RarityIndex) const
{
	// RarityIndex: 0=Common, 1=Rare, 2=Epic, 3=Legendary, -1/그 외=기본(Common)
	switch (RarityIndex)
	{
	case 1:  return RareStyle;
	case 2:  return EpicStyle;
	case 3:  return LegendaryStyle;
	default: return CommonStyle;
	}
}
