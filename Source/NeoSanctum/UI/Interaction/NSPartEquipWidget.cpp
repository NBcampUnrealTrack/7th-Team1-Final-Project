// Copyright 2026 One Team. All rights reserved.


#include "NSPartEquipWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"

void UNSPartEquipWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	OwningController = Interactor;
	AddToViewport();

	Interactor->SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Interactor->SetInputMode(InputMode);
}

void UNSPartEquipWidget::EquipSelectedPart(int32 SelectableIndex)
{
	if (!SelectableParts.IsValidIndex(SelectableIndex))
	{
		return;
	}

	const TSoftObjectPtr<UNSPartDefinition> SoftDef = SelectableParts[SelectableIndex];
	if (SoftDef.IsNull())
	{
		return;
	}

	TWeakObjectPtr<UNSPartEquipWidget> WeakThis(this);
	UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SoftDef.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda([WeakThis, SoftDef]()
			{
					UNSPartEquipWidget* StrongThis = WeakThis.Get();
					if (!StrongThis)
					{
							return;
					}
					UNSPartDefinition* Def = SoftDef.Get();
					if (!Def)
					{
							return;
					}

					UNSPartEquipComponent* Equip = StrongThis->GetEquipComponent();
					if (!Equip)
					{
							return;
					}

					// 아웃런 규칙: Common 등급, 부위는 정의의 슬롯
					FNSPartData NewPart;
					NewPart.DefinitionPtr = Def;
					NewPart.Slot = Def->PartSlot;
					NewPart.CurrentRarity = ENSPartRarity::Common;
					NewPart.RollCount = 0;

					const FNSPartValueRange* Range = Def->ValueRange.Find(ENSPartRarity::Common);
					NewPart.CurrentValue = Range ? Range->Min : 0.f;

					Equip->Server_RequestEquip(NewPart);
					StrongThis->bDirty = true;
			}));
}

void UNSPartEquipWidget::RequestClose()
{
	if (bDirty)
	{
		ShowSaveConfirmDialog();
		return;
	}
	CloseWidget();
}

void UNSPartEquipWidget::CloseWidget()
{
	if (APlayerController* PC = OwningController.Get())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}

bool UNSPartEquipWidget::GetEquippedDefinition(ENSPartSlot PartSlot, UNSPartDefinition*& OutDefinition) const
{
	OutDefinition = nullptr;

	UNSPartEquipComponent* Equip = GetEquipComponent();
	if (!Equip)
	{
		return false;
	}

	const FNSPartData* Data = Equip->GetEquippedPart(PartSlot);
	if (!Data || Data->DefinitionPtr.IsNull())
	{
		return false;
	}

	// 이미 로드돼 있으면 즉시 반환, 아니면 false (비주얼은 BP에서 비동기 처리)
	OutDefinition = Data->DefinitionPtr.Get();
	return OutDefinition != nullptr;
}

UNSPartEquipComponent* UNSPartEquipWidget::GetEquipComponent() const
{
	const APlayerController* PC = OwningController.Get();
	if (!PC)
	{
		return nullptr;
	}
	const APlayerState* PS = OwningController->PlayerState;
	if (!PS)
	{
		return nullptr;
	}
	return PS->FindComponentByClass<UNSPartEquipComponent>();
}
