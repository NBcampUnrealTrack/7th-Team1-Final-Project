// Copyright 2026 One Team. All rights reserved.

#include "NSPartPanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/UI/Part/Button/NSPartSlotButton.h"

void UNSPartPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindPartEquipComponent();
	RefreshEquippedParts();
}

void UNSPartPanelWidget::NativeDestruct()
{
	UnbindPartEquipComponent();

	Super::NativeDestruct();
}

void UNSPartPanelWidget::RefreshEquippedParts()
{
	ApplySlot(ENSPartSlot::Body, BodySlotButton);
	ApplySlot(ENSPartSlot::Arm, ArmSlotButton);
	ApplySlot(ENSPartSlot::Leg, LegSlotButton);
}

UNSPartEquipComponent* UNSPartPanelWidget::GetPartEquipComponent() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	ANSPlayerState* NSPlayerState = PlayerController->GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return nullptr;
	}

	return NSPlayerState->GetPartEquipComponent();
}

void UNSPartPanelWidget::BindPartEquipComponent()
{
	UNSPartEquipComponent* PartEquipComponent = GetPartEquipComponent();
	if (!IsValid(PartEquipComponent))
	{
		return;
	}

	CachedPartEquipComponent = PartEquipComponent;

	//파츠 컴포넌트는 일반 C++ 멀티캐스트 델리게이트를 사용하므로 AddUObject로 구독한다.
	PartEquipComponent->OnPartChanged.AddUObject(this, &UNSPartPanelWidget::HandlePartChanged);
}

void UNSPartPanelWidget::UnbindPartEquipComponent()
{
	if (!CachedPartEquipComponent.IsValid())
	{
		return;
	}

	CachedPartEquipComponent->OnPartChanged.RemoveAll(this);
	CachedPartEquipComponent.Reset();
}

void UNSPartPanelWidget::ApplySlot(ENSPartSlot PartSlot, UNSPartSlotButton* SlotButton)
{
	if (!IsValid(SlotButton))
	{
		return;
	}

	UNSPartEquipComponent* PartEquipComponent = GetPartEquipComponent();
	if (!IsValid(PartEquipComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartPanel] ApplySlot: PartEquipComponent 없음 (Slot=%d)"), (int32)PartSlot);
		SlotButton->ClearPart();
		return;
	}

	const FNSPartData* PartData = PartEquipComponent->GetEquippedPart(PartSlot);
	if (PartData == nullptr || !PartData->IsValid())
	{
		return;
	}

	// Definition 로드는 파츠 시스템 공통 유틸을 통해 처리해 UI와 시스템의 조회 방식을 맞춘다.
	UNSPartDefinition* PartDefinition = NSPartUtils::ResolvePartDefinition(this, *PartData);
	if (!IsValid(PartDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PartPanel] ApplySlot: Definition null → ClearPart (Slot=%d, Def=%s)"),
			(int32)PartSlot, *PartData->DefinitionPtr.ToString());
		SlotButton->ClearPart();
		return;
	}
	SlotButton->SetPart(*PartData, PartDefinition);
}
void UNSPartPanelWidget::HandlePartChanged(ENSPartSlot PartSlot, const FNSPartData& PartData)
{
	switch (PartSlot)
	{
	case ENSPartSlot::Body:
		ApplySlot(PartSlot, BodySlotButton);
		break;
	case ENSPartSlot::Arm:
		ApplySlot(PartSlot, ArmSlotButton);
		break;
	case ENSPartSlot::Leg:
		ApplySlot(PartSlot, LegSlotButton);
		break;
	default:
		break;
	}
}