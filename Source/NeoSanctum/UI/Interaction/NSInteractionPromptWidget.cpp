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
	CommonStyle.BackgroundColor    = FLinearColor(0.10f, 0.10f, 0.12f, 0.90f);
	CommonStyle.AccentColor        = FLinearColor(0.50f, 0.50f, 0.50f, 1.0f);

	RareStyle.BackgroundColor      = FLinearColor(0.04f, 0.12f, 0.06f, 0.90f);
	RareStyle.AccentColor          = FLinearColor(0.10f, 0.80f, 0.20f, 1.0f);

	EpicStyle.BackgroundColor      = FLinearColor(0.10f, 0.04f, 0.14f, 0.90f);
	EpicStyle.AccentColor          = FLinearColor(0.60f, 0.10f, 0.90f, 1.0f);

	LegendaryStyle.BackgroundColor = FLinearColor(0.14f, 0.10f, 0.02f, 0.90f);
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
