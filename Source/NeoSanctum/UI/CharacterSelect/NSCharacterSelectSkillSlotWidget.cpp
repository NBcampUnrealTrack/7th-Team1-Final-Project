// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterSelectSkillSlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "NeoSanctum/Data/UI/NSCharacterSkillUISet.h"
#include "NeoSanctum/Data/UI/NSSkillUIData.h"

void UNSCharacterSelectSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯이 다시 Construct되어도 이벤트가 중복되지 않게 정리.
	OnHovered().RemoveAll(this);
	OnUnhovered().RemoveAll(this);
	OnClicked().RemoveAll(this);

	OnHovered().AddUObject(this, &ThisClass::HandleHovered);
	OnUnhovered().AddUObject(this, &ThisClass::HandleUnhovered);
	OnClicked().AddUObject(this, &ThisClass::HandleClicked);

	SetSlotPreviewed(false);
}

void UNSCharacterSelectSkillSlotWidget::NativeDestruct()
{
	OnHovered().RemoveAll(this);
	OnUnhovered().RemoveAll(this);
	OnClicked().RemoveAll(this);

	OnSlotHovered.Clear();
	OnSlotUnhovered.Clear();
	OnSlotClicked.Clear();

	Super::NativeDestruct();
}

void UNSCharacterSelectSkillSlotWidget::SetupSlot(
	ENSCharacterSelectSkillSlot InSlotType,
	const FDataTableRowHandle& InSkillUIDataRow,
	const FNSInputDisplayData& InInputDisplayData)
{
	SlotType = InSlotType;
	SkillUIDataRow = InSkillUIDataRow;

	const FNSSkillUIData* SkillUIData =
		SkillUIDataRow.GetRow<FNSSkillUIData>(TEXT("CharacterSelectSkillSlot"));

	if (!SkillUIData)
	{
		ClearSlot();
		return;
	}

	SetIsEnabled(true);
	SetVisibility(ESlateVisibility::Visible);
	SetSlotPreviewed(false);

	if (SkillIconImage)
	{
		// 아이콘은 CommonUI 단계에서 선로드했으므로 동기 로드하지 않음.
		UTexture2D* SkillTexture = SkillUIData->CharacterSelectIcon.Get();

		SkillIconImage->SetBrushFromTexture(SkillTexture);
		SkillIconImage->SetVisibility(SkillTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bShowInputText =
		InInputDisplayData.bShowInputDisplay &&
		!InInputDisplayData.bUseInputIcon &&
		!InInputDisplayData.InputText.IsEmpty();

	if (InputText)
	{
		InputText->SetText(InInputDisplayData.InputText);
		InputText->SetVisibility(bShowInputText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bWantsInputIcon = InInputDisplayData.bShowInputDisplay && InInputDisplayData.bUseInputIcon;

	UTexture2D* InputTexture = bWantsInputIcon ? InInputDisplayData.InputIcon.Get() : nullptr;

	if (InputIconImage)
	{
		InputIconImage->SetBrushFromTexture(InputTexture);
		InputIconImage->SetVisibility(InputTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSCharacterSelectSkillSlotWidget::SetSlotPreviewed(bool bPreviewed)
{
	if (PreviewedIndicator)
	{
		PreviewedIndicator->SetVisibility(bPreviewed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UNSCharacterSelectSkillSlotWidget::ResetSlot()
{
	ClearSlot();
}

void UNSCharacterSelectSkillSlotWidget::HandleHovered()
{
	if (!SkillUIDataRow.DataTable || SkillUIDataRow.RowName.IsNone())
	{
		return;
	}

	OnSlotHovered.Broadcast(SlotType);
}

void UNSCharacterSelectSkillSlotWidget::HandleUnhovered()
{
	if (!SkillUIDataRow.DataTable || SkillUIDataRow.RowName.IsNone())
	{
		return;
	}

	// 부모 위젯은 현재 요구사항에 따라 상세 정보를 유지하고,
	// 필요한 경우 시각적 호버 상태만 해제할 수 있음.
	OnSlotUnhovered.Broadcast(SlotType);
}

void UNSCharacterSelectSkillSlotWidget::HandleClicked()
{
	if (!SkillUIDataRow.DataTable || SkillUIDataRow.RowName.IsNone())
	{
		return;
	}

	OnSlotClicked.Broadcast(SlotType);
}

void UNSCharacterSelectSkillSlotWidget::ClearSlot()
{
	SkillUIDataRow = FDataTableRowHandle();

	SetSlotPreviewed(false);
	SetIsEnabled(false);
	SetVisibility(ESlateVisibility::Collapsed);

	if (SkillIconImage)
	{
		SkillIconImage->SetBrushFromTexture(nullptr);
		SkillIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (InputText)
	{
		InputText->SetText(FText::GetEmpty());
		InputText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (InputIconImage)
	{
		InputIconImage->SetBrushFromTexture(nullptr);
		InputIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}
