// Copyright 2026 One Team. All rights reserved.


#include "NSPartSlotButton.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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
		PartIconImage->SetBrushFromSoftTexture(InPartDefinition->Icon);
		PartIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(PartNameText))
	{
		PartNameText->SetText(InPartDefinition->PartName);
		PartNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (IsValid(PartValueText))
	{
		PartValueText->SetText(FText::Format(
			NSLOCTEXT("PartSlotButton", "PartValueFormat", "{0} {1}"),
			GetRarityText(InPartData.CurrentRarity),
			FText::AsNumber(InPartData.CurrentValue)
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