// Copyright 2026 One Team. All rights reserved.


#include "NSPartEquipWidget.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"

static UNSProgressionSubsystem* GetProgressionSS(const UObject* WorldCtx)
{
	const UGameInstance* GI = WorldCtx ? WorldCtx->GetWorld()->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
}

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

bool UNSPartEquipWidget::RequestUnlockPart(TSoftObjectPtr<UNSPartDefinition> Definition)
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	if (!SS)
	{
		return false;
	}
	const bool bSuccess = SS->PurchasePart(Definition, ENSPartRarity::Common);
	if (bSuccess)
	{
		bDirty = true;
	}
	return bSuccess;
}

void UNSPartEquipWidget::RequestEquipPart(TSoftObjectPtr<UNSPartDefinition> Definition)
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	if (!SS)
	{
		return;
	}
	const FName CharId = SS->GetLastSelectedCharacterId();
	SS->SetEquippedPart(CharId, Definition, ENSPartRarity::Common);
	bDirty = true;
}

bool UNSPartEquipWidget::IsPartOwned(TSoftObjectPtr<UNSPartDefinition> Definition) const
{
	const UNSProgressionSubsystem* SS = GetProgressionSS(this);
	return SS ? SS->IsPartOwned(Definition, ENSPartRarity::Common) : false;
}

FNSPartSaveData UNSPartEquipWidget::GetEquippedPart() const
{
	const UNSProgressionSubsystem* SS = GetProgressionSS(this);
	if (!SS)
	{
		return FNSPartSaveData();
	}
	return SS->GetEquippedPart(SS->GetLastSelectedCharacterId());
}

TArray<FNSPartDefinitionRow> UNSPartEquipWidget::GetAllPartRows() const
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return {};
	}
	TArray<FNSPartDefinitionRow> Result;
	for (const auto& Pair : DataSS->GetAllPartRows())
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

int64 UNSPartEquipWidget::GetCommonCurrency() const
{
	const UNSProgressionSubsystem* SS = GetProgressionSS(this);
	return SS ? SS->GetCommonCurrency() : 0;
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
