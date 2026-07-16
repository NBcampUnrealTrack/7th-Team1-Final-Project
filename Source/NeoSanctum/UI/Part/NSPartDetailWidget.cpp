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
	// 소수점은 버리고 정수로만 표시. 반올림이면 3.8이 4로 보여 실제보다 좋아 보이므로 내림 고정
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	Options.MinimumFractionalDigits = 0;
	Options.RoundingMode = ERoundingMode::ToNegativeInfinity;
	return FText::AsNumber(Value, &Options);
}


/**
 * 텍스트가 비어있으면 줄 자체를 접어서(Collapsed) 빈 줄이 남지 않게 하는 하뭇
 * 안 쓰는 필드를 텍스트만 비우면 레이아웃에 빈 줄이 그대로 남음
 * Fill말고 Auto로 지정하고 함수 사용하면 깔끔하게 빈줄 제거
 */
static void SetOptionalText(UTextBlock* TextBlock, const FText& Text)
{
	if (!IsValid(TextBlock))
	{
		return;
	}
	TextBlock->SetText(Text);
	TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UNSPartDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromDefinition(const FNSPartDefinitionRow& Row, const UNSPartDefinition* Def)
{
	const FText Name = Def ? Def->PartName : FText::GetEmpty();
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));

	SetOptionalText(SlotText, FText::Format(NSLOCTEXT("PartDetail", "SlotLabel", "슬롯 : {0}"),
		FText::FromString(GetSlotLeafName(Row.PartSlot))));

	SetOptionalText(CostText, FText::Format(NSLOCTEXT("PartDetail", "CostLabel", "비용 : {0}"),
		FText::AsNumber(Row.UnlockCost)));

	SetOptionalText(CanRerollText, Row.bCanReroll
		? NSLOCTEXT("PartDetail", "CanReroll", "리롤 : 가능")
		: NSLOCTEXT("PartDetail", "NoReroll", "리롤 : 불가"));

	SetOptionalText(RarityText, FText::GetEmpty());
	SetOptionalText(ValueText, FText::GetEmpty());
}

void UNSPartDetailWidget::SetupFromEquipped(const FNSPartSaveData& SaveData, const UNSPartDefinition* Def)
{
	const FText Name = Def ? Def->PartName : FText::GetEmpty();
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));

	SetOptionalText(SlotText, FText::GetEmpty());
	SetOptionalText(CostText, FText::GetEmpty());
	SetOptionalText(CanRerollText, FText::GetEmpty());

	SetOptionalText(RarityText, FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
		GetPartRarityText(SaveData.Rarity)));

	SetOptionalText(ValueText, FText::Format(NSLOCTEXT("PartDetail", "EffectLabel", "효과 : {0}"),
		FormatEffectValue(SaveData.Value)));

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromInstance(const FNSPartData& Part, const UNSPartDefinition* Def)
{
	const FText Name = Def ? Def->PartName : FText::GetEmpty();
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));

	SetOptionalText(SlotText, FText::Format(NSLOCTEXT("PartDetail", "SlotLabel", "슬롯 : {0}"),
		FText::FromString(GetSlotLeafName(Part.Slot))));

	SetOptionalText(CostText, FText::GetEmpty());
	SetOptionalText(CanRerollText, FText::GetEmpty());

	SetOptionalText(RarityText, FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
		GetPartRarityText(Part.CurrentRarity)));

	SetOptionalText(ValueText, FText::Format(NSLOCTEXT("PartDetail", "EffectLabel", "효과 : {0}"),
		FormatEffectValue(Part.CurrentValue)));

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromSlotLock(const FNSPartSlotRow& SlotRow)
{
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "SlotLockNameLabel", "슬롯 : {0}"),
		FText::FromString(GetSlotLeafName(SlotRow.SlotTag))));

	SetOptionalText(SlotText, FText::GetEmpty());

	SetOptionalText(CostText, FText::Format(NSLOCTEXT("PartDetail", "UnlockCostLabel", "해금 비용 : {0}"),
		FText::AsNumber(SlotRow.UnlockCost)));

	SetOptionalText(CanRerollText, FText::GetEmpty());
	SetOptionalText(RarityText, FText::GetEmpty());
	SetOptionalText(ValueText, FText::GetEmpty());

	ClearPreview();
}

void UNSPartDetailWidget::ClearDetail()
{
	SetOptionalText(PartNameText, FText::GetEmpty());
	SetOptionalText(SlotText, FText::GetEmpty());
	SetOptionalText(CostText, FText::GetEmpty());
	SetOptionalText(CanRerollText, FText::GetEmpty());
	SetOptionalText(RarityText, FText::GetEmpty());
	SetOptionalText(ValueText, FText::GetEmpty());

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
