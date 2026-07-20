// Copyright 2026 One Team. All rights reserved.

#include "NSPartDetailWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"

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

// 스탯 표시 DT에서 이름 조회, 등록 안 된 스탯이면 빈 텍스트 (호출부가 폴백 처리)
static FText GetStatDisplayName(const UObject* WorldContextObject, const FGameplayTag& StatTag)
{
	if (const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(WorldContextObject))
	{
		if (const FNSStatDisplayInfoRow* StatInfo = DataSS->FindStatDisplayInfoRow(StatTag))
		{
			return StatInfo->DisplayName;
		}
	}
	return FText::GetEmpty();
}

// "효과 : {스탯이름} {수치}(단위 + 증가/감소)" 포맷. 스탯 이름을 못 찾으면 기존처럼 수치만 표시
static FText FormatEffectLabel(const UObject* WorldContextObject, const FGameplayTag& StatTag, const FText& StatName, float Value)
{
	const FText ValueText = NSPartUtils::FormatStatValueText(WorldContextObject, StatTag, Value);
	return StatName.IsEmpty()
		? FText::Format(NSLOCTEXT("PartDetail", "EffectLabel", "효과 : {0}"), ValueText)
		: FText::Format(NSLOCTEXT("PartDetail", "EffectLabelWithStat", "효과 : {0} {1}"), StatName, ValueText);
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

	/**
	 * 아웃런 구매는 항상 Common 등급 + StatTags 후보 중 첫 번째 스탯의 범위 최대값으로 고정된다 (NSProgressionSubsystem::PurchasePart)
	 * 그러니 구매 전에도 확정 수치를 그대로 보여줄 수 있다
	 */
	SetOptionalText(RarityText, FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
		GetPartRarityText(ENSPartRarity::Common)));

	FGameplayTag StatTag;
	FNSPartValueRange Range;
	bool bHasRange = false;
	const TArray<FGameplayTag> EligibleStatTags =
		NSPartUtils::FilterStatTagsByRarity(this, Row.StatTags, ENSPartRarity::Common);
	if (EligibleStatTags.Num() > 0)
	{
		StatTag = EligibleStatTags[0];
		bHasRange = NSPartUtils::GetStatValueRange(this, StatTag, ENSPartRarity::Common, Range);
	}

	SetOptionalText(ValueText, bHasRange
		? FormatEffectLabel(this, StatTag, GetStatDisplayName(this, StatTag), Range.Max)
		: FText::GetEmpty());
}

void UNSPartDetailWidget::SetupFromEquipped(const FNSPartSaveData& SaveData, const UNSPartDefinition* Def)
{
	const FText Name = Def ? Def->PartName : FText::GetEmpty();
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));

	SetOptionalText(SlotText, FText::GetEmpty());
	SetOptionalText(CostText, FText::GetEmpty());

	SetOptionalText(RarityText, FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
		GetPartRarityText(SaveData.Rarity)));

	/**
	 * 저장 데이터엔 StatTag가 없으므로 Definition Row의 StatTags를 등급 필터링해서 첫 값 사용
	 * (NSPartUtils::GetPartStatTag의 폴백 로직과 동일한 규칙)
	 */
	FGameplayTag StatTag;
	if (const FNSPartDefinitionRow* Row = Def
		? NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId())
		: nullptr)
	{
		const TArray<FGameplayTag> EligibleStatTags =
			NSPartUtils::FilterStatTagsByRarity(this, Row->StatTags, SaveData.Rarity);
		if (EligibleStatTags.Num() > 0)
		{
			StatTag = EligibleStatTags[0];
		}
	}

	SetOptionalText(ValueText, FormatEffectLabel(this, StatTag, GetStatDisplayName(this, StatTag), SaveData.Value));

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromInstance(const FNSPartData& Part, const UNSPartDefinition* Def, int64 Price)
{
	const FText Name = Def ? Def->PartName : FText::GetEmpty();
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "NameLabel", "이름 : {0}"), Name));

	SetOptionalText(SlotText, FText::Format(NSLOCTEXT("PartDetail", "SlotLabel", "슬롯 : {0}"),
		FText::FromString(GetSlotLeafName(Part.Slot))));

	SetOptionalText(CostText, Price >= 0
		? FText::Format(NSLOCTEXT("PartDetail", "CostLabel", "비용 : {0}"), FText::AsNumber(Price))
		: FText::GetEmpty());

	SetOptionalText(RarityText, FText::Format(NSLOCTEXT("PartDetail", "RarityLabel", "등급 : {0}"),
		GetPartRarityText(Part.CurrentRarity)));

	const FGameplayTag StatTag = NSPartUtils::GetPartStatTag(this, Part);
	SetOptionalText(ValueText, FormatEffectLabel(this, StatTag, GetStatDisplayName(this, StatTag), Part.CurrentValue));

	ClearPreview();
}

void UNSPartDetailWidget::SetupFromSlotLock(const FNSPartSlotRow& SlotRow)
{
	SetOptionalText(PartNameText, FText::Format(NSLOCTEXT("PartDetail", "SlotLockNameLabel", "슬롯 : {0}"),
		FText::FromString(GetSlotLeafName(SlotRow.SlotTag))));

	SetOptionalText(SlotText, FText::GetEmpty());

	SetOptionalText(CostText, FText::Format(NSLOCTEXT("PartDetail", "UnlockCostLabel", "해금 비용 : {0}"),
		FText::AsNumber(SlotRow.UnlockCost)));

	SetOptionalText(RarityText, FText::GetEmpty());
	SetOptionalText(ValueText, FText::GetEmpty());

	ClearPreview();
}

void UNSPartDetailWidget::ClearDetail()
{
	SetOptionalText(PartNameText, FText::GetEmpty());
	SetOptionalText(SlotText, FText::GetEmpty());
	SetOptionalText(CostText, FText::GetEmpty());
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
