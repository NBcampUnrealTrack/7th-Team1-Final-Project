// Copyright 2026 One Team. All rights reserved.

#include "NSPartDetailWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"

static FText GetPartRarityText(ENSPartRarity Rarity)
{
	switch (Rarity)
	{
	case ENSPartRarity::Common:
		return NSLOCTEXT("PartDetail", "CommonRarity", "Common");
	case ENSPartRarity::Rare:
		return NSLOCTEXT("PartDetail", "RareRarity", "Rare");
	case ENSPartRarity::Epic:
		return NSLOCTEXT("PartDetail", "EpicRarity", "Epic");
	case ENSPartRarity::Legendary:
		return NSLOCTEXT("PartDetail", "LegendaryRarity", "Legendary");
	default:
		break;
	}

	return FText::GetEmpty();
}

// FGameplayTag "Part.Slot.Leg" -> "Leg"
static FString GetSlotLeafName(FGameplayTag SlotTag)
{
	FString TagString = SlotTag.ToString();
	int32 LastDotIndex = INDEX_NONE;
	if (TagString.FindLastChar(TEXT('.'), LastDotIndex))
	{
		return TagString.Mid(LastDotIndex + 1);
	}
	return TagString;
}

static FText FormatEffectValue(float Value)
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;
	Options.MinimumFractionalDigits = 1;
	return FText::AsNumber(Value, &Options);
}

void UNSPartDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromDefinition(const FNSPartDefinitionRow& Row, const UNSPartDefinition* Def)
{
	if (IsValid(PartNameText))
	{
		const FText Name = Def ? Def->PartName : FText::GetEmpty();
		PartNameText->SetText(FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));
	}

	if (IsValid(SlotText))
	{
		SlotText->SetText(FText::Format(NSLOCTEXT("PartDetail", "SlotLabel", "슬롯 : {0}"),
			FText::FromString(GetSlotLeafName(Row.PartSlot))));
	}

	if (IsValid(CostText))
	{
		CostText->SetText(FText::Format(NSLOCTEXT("PartDetail", "CostLabel", "비용 : {0}"),
			FText::AsNumber(Row.UnlockCost)));
	}

	if (IsValid(CanRerollText))
	{
		CanRerollText->SetText(Row.bCanReroll
			? NSLOCTEXT("PartDetail", "CanReroll", "리롤 : 가능")
			: NSLOCTEXT("PartDetail", "NoReroll", "리롤 : 불가"));
	}

	if (IsValid(RarityText))
	{
		RarityText->SetText(FText::GetEmpty());
	}

	if (IsValid(ValueText))
	{
		ValueText->SetText(FText::GetEmpty());
	}
}

void UNSPartDetailWidget::SetupFromEquipped(const FNSPartSaveData& SaveData, const UNSPartDefinition* Def)
{
	if (IsValid(PartNameText))
	{
		const FText Name = Def ? Def->PartName : FText::GetEmpty();
		PartNameText->SetText(FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));
	}

	if (IsValid(SlotText))
	{
		SlotText->SetText(FText::GetEmpty());
	}

	if (IsValid(CostText))
	{
		CostText->SetText(FText::GetEmpty());
	}

	if (IsValid(CanRerollText))
	{
		CanRerollText->SetText(FText::GetEmpty());
	}

	if (IsValid(RarityText))
	{
		RarityText->SetText(FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
			GetPartRarityText(SaveData.Rarity)));
	}

	if (IsValid(ValueText))
	{
		ValueText->SetText(FText::Format(NSLOCTEXT("PartDetail", "EffectLabel", "효과 : {0}"),
			FormatEffectValue(SaveData.Value)));
	}

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromSlotLock(const FNSPartSlotRow& SlotRow)
{
	if (IsValid(PartNameText))
	{
		PartNameText->SetText(FText::Format(NSLOCTEXT("PartDetail", "SlotLockNameLabel", "슬롯 : {0}"),
			FText::FromString(GetSlotLeafName(SlotRow.SlotTag))));
	}

	if (IsValid(SlotText))
	{
		SlotText->SetText(FText::GetEmpty());
	}

	if (IsValid(CostText))
	{
		CostText->SetText(FText::Format(NSLOCTEXT("PartDetail", "UnlockCostLabel", "해금 비용 : {0}"),
			FText::AsNumber(SlotRow.UnlockCost)));
	}

	if (IsValid(CanRerollText))
	{
		CanRerollText->SetText(FText::GetEmpty());
	}

	if (IsValid(RarityText))
	{
		RarityText->SetText(FText::GetEmpty());
	}

	if (IsValid(ValueText))
	{
		ValueText->SetText(FText::GetEmpty());
	}

	ClearPreview();
}

void UNSPartDetailWidget::ClearDetail()
{
	if (IsValid(PartNameText))
	{
		PartNameText->SetText(FText::GetEmpty());
	}

	if (IsValid(SlotText))
	{
		SlotText->SetText(FText::GetEmpty());
	}

	if (IsValid(CostText))
	{
		CostText->SetText(FText::GetEmpty());
	}

	if (IsValid(CanRerollText))
	{
		CanRerollText->SetText(FText::GetEmpty());
	}

	if (IsValid(RarityText))
	{
		RarityText->SetText(FText::GetEmpty());
	}

	if (IsValid(ValueText))
	{
		ValueText->SetText(FText::GetEmpty());
	}

	ClearPreview();
}

void UNSPartDetailWidget::SetPreviewTarget(ANSPartPreviewStage* Stage)
{
	PreviewStageRef = Stage;

	UTextureRenderTarget2D* RenderTarget = Stage ? Stage->GetRenderTarget() : nullptr;
	if (!IsValid(PreviewImage) || !RenderTarget || !PreviewMaterialBase)
	{
		ClearPreview();
		return;
	}

	if (!IsValid(PreviewMID))
	{
		PreviewMID = UMaterialInstanceDynamic::Create(PreviewMaterialBase, this);
		PreviewImage->SetBrushFromMaterial(PreviewMID);
	}

	PreviewMID->SetTextureParameterValue(TEXT("Texture"), RenderTarget);
	PreviewImage->SetVisibility(ESlateVisibility::Visible);
}

void UNSPartDetailWidget::ClearPreview()
{
	bIsDraggingPreview = false;
	if (IsValid(PreviewImage))
	{
		PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FReply UNSPartDetailWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& IsValid(PreviewImage)
		&& PreviewImage->GetVisibility() == ESlateVisibility::Visible
		&& PreviewImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bIsDraggingPreview = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UNSPartDetailWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingPreview)
	{
		if (ANSPartPreviewStage* Stage = PreviewStageRef.Get())
		{
			Stage->AddManualYaw(InMouseEvent.GetCursorDelta().X * PreviewDragSensitivity);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UNSPartDetailWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingPreview && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UNSPartDetailWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsValid(PreviewImage)
		&& PreviewImage->GetVisibility() == ESlateVisibility::Visible
		&& PreviewImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		if (ANSPartPreviewStage* Stage = PreviewStageRef.Get())
		{
			Stage->AddZoom(InMouseEvent.GetWheelDelta());
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}
