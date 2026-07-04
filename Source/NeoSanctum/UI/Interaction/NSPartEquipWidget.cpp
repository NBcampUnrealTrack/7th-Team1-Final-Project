// Copyright 2026 One Team. All rights reserved.


#include "NSPartEquipWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/UI/Part/Button/NSPartSlotButton.h"
#include "NeoSanctum/UI/Part/NSPartCatalogEntryWidget.h"
#include "NeoSanctum/UI/Part/NSPartDetailWidget.h"

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

	BuildPartEntries();

	if (IsValid(BodyEquippedButton))
	{
		BodyEquippedButton->OnClicked().AddUObject(this, &UNSPartEquipWidget::OnBodyEquippedClicked);
	}
	if (IsValid(ArmEquippedButton))
	{
		ArmEquippedButton->OnClicked().AddUObject(this, &UNSPartEquipWidget::OnArmEquippedClicked);
	}
	if (IsValid(LegEquippedButton))
	{
		LegEquippedButton->OnClicked().AddUObject(this, &UNSPartEquipWidget::OnLegEquippedClicked);
	}
	if (IsValid(EquipButton))
	{
		EquipButton->OnClicked.AddUniqueDynamic(this, &UNSPartEquipWidget::OnEquipButtonClicked);
	}

	SelectionMode = ENSPartSelectionMode::None;
	if (IsValid(SelectedDetailWidget))
	{
		SelectedDetailWidget->ClearDetail();
	}
	RefreshEquipButton();
	RefreshEquippedDisplay();

	if (PreviewStageClass && !IsValid(PreviewStage))
	{
		if (UWorld* World = GetWorld())
		{
			const FTransform SpawnTransform(FVector(0.f, 0.f, -1000.f));
			PreviewStage = World->SpawnActor<ANSPartPreviewStage>(PreviewStageClass, SpawnTransform);
		}
	}

	Interactor->SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Interactor->SetInputMode(InputMode);
}

bool UNSPartEquipWidget::RequestUnlockSlot(FGameplayTag PartSlot)
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	if (!SS)
	{
		return false;
	}
	const bool bSuccess = SS->UnlockSlot(SS->GetLastSelectedCharacterId(), PartSlot);
	if (bSuccess)
	{
		bDirty = true;
	}
	return bSuccess;
}

bool UNSPartEquipWidget::IsSlotUnlocked(FGameplayTag PartSlot) const
{
	const UNSProgressionSubsystem* SS = GetProgressionSS(this);
	return SS ? SS->IsSlotUnlocked(SS->GetLastSelectedCharacterId(), PartSlot) : false;
}

bool UNSPartEquipWidget::IsSlotEquipped(FGameplayTag PartSlot) const
{
	const FNSPartSaveData EquippedSave = GetEquippedPart();
	const UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, EquippedSave.Definition);
	if (!IsValid(Def))
	{
		return false;
	}

	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId());
	return Row && Row->PartSlot == PartSlot;
}

int64 UNSPartEquipWidget::GetSlotUnlockCost(FGameplayTag PartSlot) const
{
	const UNSProgressionSubsystem* SS = GetProgressionSS(this);
	return SS ? SS->GetSlotUnlockCost(PartSlot) : 0;
}

TArray<FNSPartSlotRow> UNSPartEquipWidget::GetAllSlotRows() const
{
	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return {};
	}
	TArray<FNSPartSlotRow> Result;
	for (const auto& Pair : DataSS->GetAllSlotRows())
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

bool UNSPartEquipWidget::RequestUnlockPart(TSoftObjectPtr<UNSPartDefinition> Definition)
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	if (!SS)
	{
		NS_LOG(LogNS, Warning, "[Equip] RequestUnlockPart 실패: ProgressionSubsystem이 없습니다.");
		return false;
	}
	const bool bSuccess = SS->PurchasePart(SS->GetLastSelectedCharacterId(), Definition, ENSPartRarity::Common);
	NS_LOG(LogNS, Log, "[Equip] PurchasePart 결과: {Success} ({Definition})",
		("Success", bSuccess), ("Definition", Definition.ToString()));
	if (bSuccess)
	{
		bDirty = true;
	}
	return bSuccess;
}

void UNSPartEquipWidget::RequestEquipPart(TSoftObjectPtr<UNSPartDefinition> Definition)
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	ANSPlayerController* PC = Cast<ANSPlayerController>(OwningController.Get());
	if (!SS || !PC)
	{
		NS_LOG(LogNS, Warning, "[Equip] RequestEquipPart 실패: ProgressionSubsystem 또는 PlayerController가 없습니다.");
		return;
	}
	const FName CharId = SS->GetLastSelectedCharacterId();
	// 로컬 저장 + 서버 업로드 + 현재 폰 즉시 적용까지 한 번에 처리
	PC->EquipPartLive(CharId, Definition, ENSPartRarity::Common);
	NS_LOG(LogNS, Log, "[Equip] EquipPartLive 호출: CharId={CharId}, {Definition}",
		("CharId", CharId.ToString()), ("Definition", Definition.ToString()));
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

void UNSPartEquipWidget::SelectCatalogPart(const FNSPartDefinitionRow& Row, UNSPartCatalogEntryWidget* SourceEntry)
{
	NS_LOG(LogNS, Log, "[Equip] SelectCatalogPart 호출됨: {Definition}", ("Definition", Row.Definition.ToString()));

	SelectedRow = Row;
	SelectionMode = ENSPartSelectionMode::Part;

	if (IsValid(SelectedCatalogEntryWidget.Get()) && SelectedCatalogEntryWidget.Get() != SourceEntry)
	{
		SelectedCatalogEntryWidget->SetIsSelected(false);
	}
	SelectedCatalogEntryWidget = SourceEntry;
	if (IsValid(SourceEntry))
	{
		SourceEntry->SetIsSelected(true);
	}

	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, Row.Definition);

	if (!IsValid(SelectedDetailWidget))
	{
		NS_LOG(LogNS, Warning, "[Equip] SelectedDetailWidget 바인딩이 유효하지 않습니다. WBP에서 SelectedDetailWidget 이름을 확인하세요.");
	}
	else if (IsValid(Def))
	{
		NS_LOG(LogNS, Log, "[Equip] Definition Resolve 성공, SetupFromDefinition 호출");
		SelectedDetailWidget->SetupFromDefinition(Row, Def);
	}
	else
	{
		NS_LOG(LogNS, Warning, "[Equip] Definition Resolve 실패, ClearDetail 호출: {Definition}", ("Definition", Row.Definition.ToString()));
		SelectedDetailWidget->ClearDetail();
	}

	if (PreviewMeshLoadHandle.IsValid())
	{
		PreviewMeshLoadHandle->CancelHandle();
		PreviewMeshLoadHandle.Reset();
	}

	if (IsValid(PreviewStage))
	{
		if (IsValid(Def) && !Def->PartMesh.IsNull())
		{
			if (IsValid(SelectedDetailWidget))
			{
				SelectedDetailWidget->SetPreviewTarget(PreviewStage);
			}

			TWeakObjectPtr<UNSPartEquipWidget> WeakThis(this);
			TSoftObjectPtr<USkeletalMesh> SoftMesh = Def->PartMesh;
			PreviewMeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
				SoftMesh.ToSoftObjectPath(),
				[WeakThis, SoftMesh]()
				{
					UNSPartEquipWidget* StrongThis = WeakThis.Get();
					if (!StrongThis || !IsValid(StrongThis->PreviewStage))
					{
						return;
					}
					StrongThis->PreviewStage->SetPreviewMesh(SoftMesh.Get());
					StrongThis->PreviewMeshLoadHandle.Reset();
				});
		}
		else
		{
			PreviewStage->SetPreviewMesh(nullptr);
			if (IsValid(SelectedDetailWidget))
			{
				SelectedDetailWidget->ClearPreview();
			}
		}
	}

	RefreshSelectionHighlights();
	RefreshEquipButton();
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

	if (PreviewMeshLoadHandle.IsValid())
	{
		PreviewMeshLoadHandle->CancelHandle();
		PreviewMeshLoadHandle.Reset();
	}

	if (IsValid(PreviewStage))
	{
		PreviewStage->Destroy();
		PreviewStage = nullptr;
	}

	RemoveFromParent();
}

UPanelWidget* UNSPartEquipWidget::GetCatalogContainerForSlot(FGameplayTag SlotTag) const
{
	if (SlotTag == BodySlotTag)
	{
		return BodyListContainer;
	}
	if (SlotTag == ArmSlotTag)
	{
		return ArmListContainer;
	}
	if (SlotTag == LegSlotTag)
	{
		return LegListContainer;
	}
	return nullptr;
}

void UNSPartEquipWidget::BuildPartEntries()
{
	if (!PartEntryTemplate)
	{
		return;
	}

	if (IsValid(BodyListContainer))
	{
		BodyListContainer->ClearChildren();
	}
	if (IsValid(ArmListContainer))
	{
		ArmListContainer->ClearChildren();
	}
	if (IsValid(LegListContainer))
	{
		LegListContainer->ClearChildren();
	}

	const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	if (!DataSS)
	{
		return;
	}

	for (const auto& Pair : DataSS->GetAllPartRows())
	{
		if (!Pair.Value.bEnabled)
		{
			continue;
		}

		UPanelWidget* Container = GetCatalogContainerForSlot(Pair.Value.PartSlot);
		if (!IsValid(Container))
		{
			continue;
		}

		UNSPartCatalogEntryWidget* Entry = CreateWidget<UNSPartCatalogEntryWidget>(this, PartEntryTemplate);
		if (!Entry)
		{
			continue;
		}

		Entry->SetupEntry(Pair.Value, this);
		Container->AddChild(Entry);
	}
}

void UNSPartEquipWidget::RefreshEquippedDisplay()
{
	const FNSPartSaveData EquippedSave = GetEquippedPart();
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, EquippedSave.Definition);

	if (IsValid(EquippedDetailWidget))
	{
		if (IsValid(Def))
		{
			EquippedDetailWidget->SetupFromEquipped(EquippedSave, Def);
		}
		else
		{
			EquippedDetailWidget->ClearDetail();
		}
	}

	FGameplayTag EquippedSlot;
	if (IsValid(Def))
	{
		const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId());
		if (Row)
		{
			EquippedSlot = Row->PartSlot;
		}
	}

	FNSPartData PartData;
	PartData.DefinitionPtr = EquippedSave.Definition;
	PartData.CurrentRarity = EquippedSave.Rarity;
	PartData.CurrentValue = EquippedSave.Value;

	RefreshEquippedSlotButton(BodyEquippedButton, BodySlotTag, EquippedSlot, PartData, Def);
	RefreshEquippedSlotButton(ArmEquippedButton, ArmSlotTag, EquippedSlot, PartData, Def);
	RefreshEquippedSlotButton(LegEquippedButton, LegSlotTag, EquippedSlot, PartData, Def);
}

void UNSPartEquipWidget::RefreshEquippedSlotButton(UNSPartSlotButton* Button, FGameplayTag SlotTag,
	FGameplayTag EquippedSlot, const FNSPartData& EquippedPartData, UNSPartDefinition* EquippedDef)
{
	if (!IsValid(Button))
	{
		return;
	}

	if (EquippedSlot.IsValid() && EquippedSlot == SlotTag)
	{
		FNSPartData SlotPartData = EquippedPartData;
		SlotPartData.Slot = SlotTag;
		Button->SetPart(SlotPartData, EquippedDef);
	}
	else
	{
		Button->ClearPart();
	}
}

void UNSPartEquipWidget::RefreshEquipButton()
{
	if (!IsValid(EquipButton))
	{
		return;
	}

	switch (SelectionMode)
	{
	case ENSPartSelectionMode::None:
		EquipButton->SetIsEnabled(false);
		return;

	case ENSPartSelectionMode::SlotUnlock:
		EquipButton->SetIsEnabled(true);
		if (IsValid(EquipButtonText))
		{
			EquipButtonText->SetText(NSLOCTEXT("PartEquip", "UnlockSlot", "슬롯 해금"));
		}
		return;

	case ENSPartSelectionMode::UnequipPart:
		EquipButton->SetIsEnabled(true);
		if (IsValid(EquipButtonText))
		{
			EquipButtonText->SetText(NSLOCTEXT("PartEquip", "Unequip", "해제"));
		}
		return;

	case ENSPartSelectionMode::Part:
		if (!IsSlotUnlocked(SelectedRow.PartSlot))
		{
			EquipButton->SetIsEnabled(false);
			if (IsValid(EquipButtonText))
			{
				EquipButtonText->SetText(NSLOCTEXT("PartEquip", "SlotUnlockRequired", "슬롯 해금 필요"));
			}
			return;
		}

		EquipButton->SetIsEnabled(true);
		if (IsValid(EquipButtonText))
		{
			if (!IsPartOwned(SelectedRow.Definition))
			{
				EquipButtonText->SetText(NSLOCTEXT("PartEquip", "Buy", "구매"));
			}
			else if (GetEquippedPart().Definition == SelectedRow.Definition)
			{
				// 선택한 카탈로그 파츠가 현재 장착중인 파츠면 해제로 동작
				EquipButtonText->SetText(NSLOCTEXT("PartEquip", "Unequip", "해제"));
			}
			else
			{
				EquipButtonText->SetText(NSLOCTEXT("PartEquip", "Equip", "장착"));
			}
		}
		return;
	}
}

void UNSPartEquipWidget::RequestUnequipPart()
{
	UNSProgressionSubsystem* SS = GetProgressionSS(this);
	ANSPlayerController* PC = Cast<ANSPlayerController>(OwningController.Get());
	if (!SS || !PC)
	{
		NS_LOG(LogNS, Warning, "[Equip] RequestUnequipPart 실패: ProgressionSubsystem 또는 PlayerController가 없습니다.");
		return;
	}
	const FName CharId = SS->GetLastSelectedCharacterId();
	// 로컬 저장 + 서버 업로드 + 현재 폰 즉시 적용까지 한 번에 처리
	PC->EquipPartLive(CharId, TSoftObjectPtr<UNSPartDefinition>(), ENSPartRarity::Common);
	NS_LOG(LogNS, Log, "[Equip] EquipPartLive 호출(해제): CharId={CharId}", ("CharId", CharId.ToString()));
	bDirty = true;
}

void UNSPartEquipWidget::OnEquipButtonClicked()
{
	switch (SelectionMode)
	{
	case ENSPartSelectionMode::SlotUnlock:
		if (RequestUnlockSlot(SelectedSlotForUnlock.SlotTag))
		{
			SelectionMode = ENSPartSelectionMode::None;
			SelectedSlotTag = FGameplayTag();
			if (IsValid(SelectedDetailWidget))
			{
				SelectedDetailWidget->ClearDetail();
			}
		}
		break;

	case ENSPartSelectionMode::UnequipPart:
		RequestUnequipPart();
		RefreshEquippedDisplay();
		SelectionMode = ENSPartSelectionMode::None;
		SelectedSlotTag = FGameplayTag();
		if (IsValid(SelectedDetailWidget))
		{
			SelectedDetailWidget->ClearDetail();
		}
		break;

	case ENSPartSelectionMode::Part:
		if (IsPartOwned(SelectedRow.Definition))
		{
			// 이미 장착중인 파츠를 다시 누르면 해제
			if (GetEquippedPart().Definition == SelectedRow.Definition)
			{
				RequestUnequipPart();
			}
			else
			{
				RequestEquipPart(SelectedRow.Definition);
			}
			RefreshEquippedDisplay();
		}
		else
		{
			RequestUnlockPart(SelectedRow.Definition);
		}
		break;

	case ENSPartSelectionMode::None:
		break;
	}

	RefreshSelectionHighlights();
	RefreshEquipButton();
}

void UNSPartEquipWidget::OnBodyEquippedClicked()
{
	OnSlotButtonClicked(BodySlotTag);
}

void UNSPartEquipWidget::OnArmEquippedClicked()
{
	OnSlotButtonClicked(ArmSlotTag);
}

void UNSPartEquipWidget::OnLegEquippedClicked()
{
	OnSlotButtonClicked(LegSlotTag);
}

void UNSPartEquipWidget::OnSlotButtonClicked(FGameplayTag SlotTag)
{
	if (!IsSlotUnlocked(SlotTag))
	{
		const UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
		const FNSPartSlotRow* Row = DataSS ? DataSS->GetSlotRow(SlotTag) : nullptr;
		if (!Row)
		{
			return;
		}

		SelectedSlotForUnlock = *Row;
		SelectionMode = ENSPartSelectionMode::SlotUnlock;
		SelectedSlotTag = SlotTag;
		if (IsValid(SelectedDetailWidget))
		{
			SelectedDetailWidget->SetupFromSlotLock(SelectedSlotForUnlock);
		}
		RefreshSelectionHighlights();
		RefreshEquipButton();
		return;
	}

	const FNSPartSaveData EquippedSave = GetEquippedPart();
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, EquippedSave.Definition);
	if (!IsValid(Def))
	{
		return;
	}

	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId());
	if (!Row || Row->PartSlot != SlotTag)
	{
		return;
	}

	SelectionMode = ENSPartSelectionMode::UnequipPart;
	SelectedSlotTag = SlotTag;
	if (IsValid(SelectedDetailWidget))
	{
		SelectedDetailWidget->SetupFromEquipped(EquippedSave, Def);
	}
	RefreshSelectionHighlights();
	RefreshEquipButton();
}

void UNSPartEquipWidget::RefreshSelectionHighlights()
{
	if (SelectionMode != ENSPartSelectionMode::Part && IsValid(SelectedCatalogEntryWidget.Get()))
	{
		SelectedCatalogEntryWidget->SetIsSelected(false);
		SelectedCatalogEntryWidget = nullptr;
	}

	const bool bSlotModeActive =
		SelectionMode == ENSPartSelectionMode::SlotUnlock || SelectionMode == ENSPartSelectionMode::UnequipPart;

	auto ApplySlotHighlight = [&](UNSPartSlotButton* Button, FGameplayTag SlotTag)
	{
		if (IsValid(Button))
		{
			Button->SetHighlighted(bSlotModeActive && SelectedSlotTag == SlotTag);
		}
	};

	ApplySlotHighlight(BodyEquippedButton, BodySlotTag);
	ApplySlotHighlight(ArmEquippedButton, ArmSlotTag);
	ApplySlotHighlight(LegEquippedButton, LegSlotTag);
}
