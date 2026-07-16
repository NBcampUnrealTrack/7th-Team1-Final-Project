// Copyright 2026 One Team. All rights reserved.


#include "NSPartSlotButton.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"

UNSPartSlotButton::UNSPartSlotButton()
	: bHasPart(false)
{
}

void UNSPartSlotButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshEmptyState();
}

void UNSPartSlotButton::SetPart(const FNSPartData& InPartData, const UNSPartDefinition* InPartDefinition)
{
	if (!IsValid(InPartDefinition))
	{
		ClearPart();
		return;
	}

	bHasPart = true;

	if (IsValid(PartIconImage))
	{
		if (UTexture2D* LoadedIcon = InPartDefinition->Icon.Get())
		{
			PartIconImage->SetBrushFromTexture(LoadedIcon);
			PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else if (!InPartDefinition->Icon.IsNull())
		{
			TWeakObjectPtr<UNSPartSlotButton> WeakThis(this);
			TSoftObjectPtr<UTexture2D> SoftIcon = InPartDefinition->Icon;
			IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
				SoftIcon.ToSoftObjectPath(),
				[WeakThis, SoftIcon]()
				{
					if (!WeakThis.IsValid())
					{
						return;
					}
					if (UTexture2D* Tex = SoftIcon.Get())
					{
						WeakThis->PartIconImage->SetBrushFromTexture(Tex);
						WeakThis->PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
					WeakThis->IconLoadHandle.Reset();
				});
		}
	}

	if (IsValid(PartNameText))
	{
		PartNameText->SetText(InPartDefinition->PartName);
		PartNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(PartValueText))
	{
		// 소수점은 버리고 정수로만 표시. 반올림이면 3.8이 4로 보여 실제보다 좋아 보이므로 내림 고정
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		Options.MinimumFractionalDigits = 0;
		Options.RoundingMode = ERoundingMode::ToNegativeInfinity;

		PartValueText->SetText(FText::Format(
			NSLOCTEXT("PartSlotButton", "PartValueFormat", "{0} {1}"),
			GetRarityText(InPartData.CurrentRarity),
			FText::AsNumber(InPartData.CurrentValue, &Options)
		));
		PartValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(RarityBorder))
	{
		RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UNSPartSlotButton::ClearPart()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	bHasPart = false;

	if (IsValid(PartIconImage))
	{
		PartIconImage->SetBrushFromTexture(nullptr);
		PartIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(PartNameText))
	{
		PartNameText->SetText(FText::GetEmpty());
		PartNameText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(PartValueText))
	{
		PartValueText->SetText(FText::GetEmpty());
		PartValueText->SetVisibility(ESlateVisibility::Hidden);
	}

	RefreshEmptyState();
}

bool UNSPartSlotButton::IsEmpty() const
{
	return !bHasPart;
}

void UNSPartSlotButton::SetHighlighted(bool bHighlighted)
{
	if (IsValid(SelectedHighlight))
	{
		SelectedHighlight->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSPartSlotButton::RefreshEmptyState()
{
	if (bHasPart)
	{
		return;
	}

	if (IsValid(RarityBorder))
	{
		RarityBorder->SetVisibility(ESlateVisibility::Hidden);
	}
}

FText UNSPartSlotButton::GetRarityText(ENSPartRarity Rarity) const
{
	switch (Rarity)
	{
	case ENSPartRarity::Common:
		return NSLOCTEXT("PartSlotButton", "CommonRarity", "Common");
	case ENSPartRarity::Rare:
		return NSLOCTEXT("PartSlotButton", "RareRarity", "Rare");
	case ENSPartRarity::Epic:
		return NSLOCTEXT("PartSlotButton", "EpicRarity", "Epic");
	case ENSPartRarity::Legendary:
		return NSLOCTEXT("PartSlotButton", "LegendaryRarity", "Legendary");
	default:
		break;
	}

	return FText::GetEmpty();
}