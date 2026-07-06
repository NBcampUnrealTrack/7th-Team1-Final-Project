// Copyright 2026 One Team. All rights reserved.

#include "NSPartUpgradeWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/UI/Part/Button/NSPartSlotButton.h"
#include "NeoSanctum/UI/Part/NSPartCatalogEntryWidget.h"
#include "NeoSanctum/UI/Part/NSPartDetailWidget.h"

namespace
{
	constexpr int32 PageIndexHub = 0;
	constexpr int32 PageIndexPurchase = 1;
	constexpr int32 PageIndexUpgrade = 2;

	FText GetRarityDisplayText(ENSPartRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity);
	}

	FText FormatSummaryValue(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 1;
		Options.MinimumFractionalDigits = 1;
		return FText::AsNumber(Value, &Options);
	}
}

void UNSPartUpgradeWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	OwningController = Interactor;
	AddToViewport();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnCloseClicked);
	}
	if (IsValid(HubPurchaseButton))
	{
		HubPurchaseButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnHubPurchaseClicked);
	}
	if (IsValid(HubUpgradeButton))
	{
		HubUpgradeButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnHubUpgradeClicked);
	}
	if (IsValid(PurchaseBackButton))
	{
		PurchaseBackButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnBackClicked);
	}
	if (IsValid(UpgradeBackButton))
	{
		UpgradeBackButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnBackClicked);
	}
	if (IsValid(BuyButton))
	{
		BuyButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnBuyClicked);
	}
	if (IsValid(RerollButton))
	{
		RerollButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnRerollClicked);
	}
	if (IsValid(UpgradeButton))
	{
		UpgradeButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnUpgradeClicked);
	}
	if (IsValid(UpgradeBodySlotButton))
	{
		UpgradeBodySlotButton->OnClicked().AddUObject(this, &UNSPartUpgradeWidget::OnUpgradeBodyClicked);
	}
	if (IsValid(UpgradeArmSlotButton))
	{
		UpgradeArmSlotButton->OnClicked().AddUObject(this, &UNSPartUpgradeWidget::OnUpgradeArmClicked);
	}
	if (IsValid(UpgradeLegSlotButton))
	{
		UpgradeLegSlotButton->OnClicked().AddUObject(this, &UNSPartUpgradeWidget::OnUpgradeLegClicked);
	}

	BindComponentDelegates();

	// 재고는 서버가 지연 생성 (이미 있으면 무시). 복제 도착 시 OnShopStockChanged로 갱신됨
	if (UNSPartEquipComponent* EquipComp = GetEquipComponent())
	{
		EquipComp->Server_RequestGenerateStock();
	}

	SelectedUpgradeSlot = FGameplayTag();
	SelectedStockIndex = INDEX_NONE;

	OpenHubPage();
	RefreshBalance();
	RefreshEquippedDisplays();
	RefreshStockEntries();
	RefreshUpgradePanels();

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

void UNSPartUpgradeWidget::CloseWidget()
{
	UnbindComponentDelegates();

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

// ================================================================
// 페이지 전환
// ================================================================

void UNSPartUpgradeWidget::OpenHubPage()
{
	if (IsValid(PageSwitcher))
	{
		PageSwitcher->SetActiveWidgetIndex(PageIndexHub);
	}
	RefreshEquippedDisplays();
}

void UNSPartUpgradeWidget::OpenPurchasePage()
{
	if (IsValid(PageSwitcher))
	{
		PageSwitcher->SetActiveWidgetIndex(PageIndexPurchase);
	}
	RefreshStockEntries();
}

void UNSPartUpgradeWidget::OpenUpgradePage()
{
	if (IsValid(PageSwitcher))
	{
		PageSwitcher->SetActiveWidgetIndex(PageIndexUpgrade);
	}
	RefreshEquippedDisplays();
	RefreshUpgradePanels();
}

// ================================================================
// 컴포넌트 접근/구독
// ================================================================

UNSPartEquipComponent* UNSPartUpgradeWidget::GetEquipComponent() const
{
	const APlayerController* PC = OwningController.Get();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	if (!PS)
	{
		return nullptr;
	}
	return PS->FindComponentByClass<UNSPartEquipComponent>();
}

UNSCurrencyComponent* UNSPartUpgradeWidget::GetCurrencyComponent() const
{
	const APlayerController* PC = OwningController.Get();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	if (!PS)
	{
		return nullptr;
	}
	return PS->FindComponentByClass<UNSCurrencyComponent>();
}

void UNSPartUpgradeWidget::BindComponentDelegates()
{
	if (UNSPartEquipComponent* EquipComp = GetEquipComponent())
	{
		PartChangedHandle = EquipComp->OnPartChanged.AddUObject(this, &UNSPartUpgradeWidget::HandlePartChanged);
		UpgradeResultHandle = EquipComp->OnUpgradeResult.AddUObject(this, &UNSPartUpgradeWidget::HandleUpgradeResult);
		StockChangedHandle = EquipComp->OnShopStockChanged.AddUObject(this, &UNSPartUpgradeWidget::HandleShopStockChanged);
	}
	if (UNSCurrencyComponent* Currency = GetCurrencyComponent())
	{
		TempChangedHandle = Currency->OnTempChanged.AddUObject(this, &UNSPartUpgradeWidget::HandleTempChanged);
	}
}

void UNSPartUpgradeWidget::UnbindComponentDelegates()
{
	if (UNSPartEquipComponent* EquipComp = GetEquipComponent())
	{
		EquipComp->OnPartChanged.Remove(PartChangedHandle);
		EquipComp->OnUpgradeResult.Remove(UpgradeResultHandle);
		EquipComp->OnShopStockChanged.Remove(StockChangedHandle);
	}
	if (UNSCurrencyComponent* Currency = GetCurrencyComponent())
	{
		Currency->OnTempChanged.Remove(TempChangedHandle);
	}
	PartChangedHandle.Reset();
	UpgradeResultHandle.Reset();
	StockChangedHandle.Reset();
	TempChangedHandle.Reset();
}

// ================================================================
// 갱신
// ================================================================

void UNSPartUpgradeWidget::RefreshBalance()
{
	const UNSCurrencyComponent* Currency = GetCurrencyComponent();
	SetBalanceText(Currency ? Currency->GetTemp() : 0);
}

void UNSPartUpgradeWidget::SetBalanceText(int64 Balance)
{
	if (!IsValid(TempBalanceText))
	{
		return;
	}
	TempBalanceText->SetText(FText::AsNumber(Balance));
}

void UNSPartUpgradeWidget::ApplySlotButtonDisplay(FGameplayTag SlotTag, UNSPartSlotButton* SlotButton) const
{
	if (!IsValid(SlotButton))
	{
		return;
	}

	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	const FNSPartData* Part = EquipComp ? EquipComp->GetEquippedPart(SlotTag) : nullptr;
	UNSPartDefinition* Def = Part ? NSPartUtils::ResolvePartDefinition(this, *Part) : nullptr;

	if (Part && Def)
	{
		SlotButton->SetPart(*Part, Def);
	}
	else
	{
		SlotButton->ClearPart();
	}
}

FText UNSPartUpgradeWidget::BuildEquippedSummaryText() const
{
	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp)
	{
		return FText::GetEmpty();
	}

	struct FSlotLabel
	{
		FGameplayTag Tag;
		FText Label;
	};
	const FSlotLabel SlotLabels[] = {
		{ BodySlotTag, NSLOCTEXT("PartUpgrade", "SlotBody", "바디") },
		{ ArmSlotTag, NSLOCTEXT("PartUpgrade", "SlotArm", "암") },
		{ LegSlotTag, NSLOCTEXT("PartUpgrade", "SlotLeg", "레그") },
	};

	TArray<FText> Lines;
	for (const FSlotLabel& SlotLabel : SlotLabels)
	{
		const FNSPartData* Part = EquipComp->GetEquippedPart(SlotLabel.Tag);
		if (!Part)
		{
			Lines.Add(FText::Format(NSLOCTEXT("PartUpgrade", "SlotEmptyLine", "{0} : 미장착"), SlotLabel.Label));
			continue;
		}

		const UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, *Part);
		const FText PartName = Def ? Def->PartName : FText::GetEmpty();
		Lines.Add(FText::Format(NSLOCTEXT("PartUpgrade", "SlotSummaryLine", "{0} : {1} ({2}) 효과 {3}"),
			SlotLabel.Label, PartName, GetRarityDisplayText(Part->CurrentRarity), FormatSummaryValue(Part->CurrentValue)));
	}

	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

void UNSPartUpgradeWidget::UpdatePreview(const UNSPartDefinition* Def, UNSPartDetailWidget* TargetDetail)
{
	if (PreviewMeshLoadHandle.IsValid())
	{
		PreviewMeshLoadHandle->CancelHandle();
		PreviewMeshLoadHandle.Reset();
	}

	if (!IsValid(PreviewStage))
	{
		return;
	}

	if (!IsValid(Def) || Def->PartMesh.IsNull())
	{
		PreviewStage->SetPreviewMesh(nullptr);
		if (IsValid(TargetDetail))
		{
			TargetDetail->ClearPreview();
		}
		return;
	}

	if (IsValid(TargetDetail))
	{
		TargetDetail->SetPreviewTarget(PreviewStage);
	}

	TWeakObjectPtr<UNSPartUpgradeWidget> WeakThis(this);
	TSoftObjectPtr<USkeletalMesh> SoftMesh = Def->PartMesh;
	PreviewMeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		SoftMesh.ToSoftObjectPath(),
		[WeakThis, SoftMesh]()
		{
			UNSPartUpgradeWidget* StrongThis = WeakThis.Get();
			if (!StrongThis || !IsValid(StrongThis->PreviewStage))
			{
				return;
			}
			StrongThis->PreviewStage->SetPreviewMesh(SoftMesh.Get());
			StrongThis->PreviewMeshLoadHandle.Reset();
		});
}

void UNSPartUpgradeWidget::RefreshSelectedSlotPreview()
{
	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	const FNSPartData* Part = (EquipComp && SelectedUpgradeSlot.IsValid())
		? EquipComp->GetEquippedPart(SelectedUpgradeSlot) : nullptr;
	const UNSPartDefinition* Def = Part ? NSPartUtils::ResolvePartDefinition(this, *Part) : nullptr;

	UpdatePreview(Def, SelectedPartDetailWidget);
}

void UNSPartUpgradeWidget::RefreshEquippedDisplays()
{
	// 슬롯버튼 표시 (허브 3개 + 구매 페이지 3개 + 업그레이드 페이지 상단 3개)
	ApplySlotButtonDisplay(BodySlotTag, HubBodySlotButton);
	ApplySlotButtonDisplay(ArmSlotTag, HubArmSlotButton);
	ApplySlotButtonDisplay(LegSlotTag, HubLegSlotButton);
	ApplySlotButtonDisplay(BodySlotTag, BodyEquippedButton);
	ApplySlotButtonDisplay(ArmSlotTag, ArmEquippedButton);
	ApplySlotButtonDisplay(LegSlotTag, LegEquippedButton);
	ApplySlotButtonDisplay(BodySlotTag, UpgradeBodySlotButton);
	ApplySlotButtonDisplay(ArmSlotTag, UpgradeArmSlotButton);
	ApplySlotButtonDisplay(LegSlotTag, UpgradeLegSlotButton);

	// 장착중인 모든 파츠 요약 (허브 + 업그레이드 페이지 좌측, 동일 내용)
	const FText Summary = BuildEquippedSummaryText();
	if (IsValid(HubEquippedSummaryText))
	{
		HubEquippedSummaryText->SetText(Summary);
	}
	if (IsValid(ListEquippedSummaryText))
	{
		ListEquippedSummaryText->SetText(Summary);
	}
}

void UNSPartUpgradeWidget::RefreshStockEntries()
{
	if (IsValid(BodyStockContainer))
	{
		BodyStockContainer->ClearChildren();
	}
	if (IsValid(ArmStockContainer))
	{
		ArmStockContainer->ClearChildren();
	}
	if (IsValid(LegStockContainer))
	{
		LegStockContainer->ClearChildren();
	}
	StockEntryWidgets.Reset();
	SelectedStockIndex = INDEX_NONE;
	if (IsValid(StockDetailWidget))
	{
		StockDetailWidget->ClearDetail();
	}
	RefreshBuyBox();

	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp || !StockEntryTemplate)
	{
		return;
	}

	const TArray<FNSPartData>& Stock = EquipComp->GetShopStock();
	for (int32 Index = 0; Index < Stock.Num(); ++Index)
	{
		const FNSPartData& Item = Stock[Index];

		UPanelWidget* Container = nullptr;
		if (Item.Slot == BodySlotTag)
		{
			Container = BodyStockContainer;
		}
		else if (Item.Slot == ArmSlotTag)
		{
			Container = ArmStockContainer;
		}
		else if (Item.Slot == LegSlotTag)
		{
			Container = LegStockContainer;
		}
		if (!IsValid(Container))
		{
			continue;
		}

		const FPrimaryAssetId DefId =
			UAssetManager::Get().GetPrimaryAssetIdForPath(Item.DefinitionPtr.ToSoftObjectPath());
		const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
		if (!Row)
		{
			continue;
		}

		UNSPartCatalogEntryWidget* Entry = CreateWidget<UNSPartCatalogEntryWidget>(GetOwningPlayer(), StockEntryTemplate);
		if (!Entry)
		{
			continue;
		}

		Entry->SetupEntry(*Row, FNSOnCatalogEntryClicked::CreateUObject(
			this, &UNSPartUpgradeWidget::OnStockEntryClicked, Index));
		Container->AddChild(Entry);
		StockEntryWidgets.Add(Entry);
	}
}

void UNSPartUpgradeWidget::RefreshBuyBox()
{
	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	const UNSCurrencyComponent* Currency = GetCurrencyComponent();
	const TArray<FNSPartData>* Stock = EquipComp ? &EquipComp->GetShopStock() : nullptr;
	const bool bValidSelection = Stock && Stock->IsValidIndex(SelectedStockIndex);

	int64 Price = -1;
	if (bValidSelection && EquipComp)
	{
		Price = EquipComp->GetShopPrice((*Stock)[SelectedStockIndex].CurrentRarity);
	}

	if (IsValid(BuyPriceText))
	{
		BuyPriceText->SetText(Price >= 0 ? FText::AsNumber(Price) : FText::GetEmpty());
	}
	if (IsValid(BuyButton))
	{
		const int64 Balance = Currency ? Currency->GetTemp() : 0;
		BuyButton->SetIsEnabled(bValidSelection && Price >= 0 && Balance >= Price);
	}
}

void UNSPartUpgradeWidget::RefreshUpgradePanels()
{
	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	const UNSCurrencyComponent* Currency = GetCurrencyComponent();
	const FNSPartData* Part = (EquipComp && SelectedUpgradeSlot.IsValid())
		? EquipComp->GetEquippedPart(SelectedUpgradeSlot) : nullptr;
	UNSPartDefinition* Def = Part ? NSPartUtils::ResolvePartDefinition(this, *Part) : nullptr;

	// 슬롯 선택 하이라이트
	if (IsValid(UpgradeBodySlotButton))
	{
		UpgradeBodySlotButton->SetHighlighted(SelectedUpgradeSlot == BodySlotTag);
	}
	if (IsValid(UpgradeArmSlotButton))
	{
		UpgradeArmSlotButton->SetHighlighted(SelectedUpgradeSlot == ArmSlotTag);
	}
	if (IsValid(UpgradeLegSlotButton))
	{
		UpgradeLegSlotButton->SetHighlighted(SelectedUpgradeSlot == LegSlotTag);
	}

	// 중앙 상세
	if (IsValid(SelectedPartDetailWidget))
	{
		if (Part && Def)
		{
			SelectedPartDetailWidget->SetupFromInstance(*Part, Def);
			UpdatePreview(Def, SelectedPartDetailWidget);
		}
		else
		{
			SelectedPartDetailWidget->ClearDetail();
		}
	}

	if (!Part || !Def || !EquipComp)
	{
		if (IsValid(RerollRangeText))
		{
			RerollRangeText->SetText(FText::GetEmpty());
		}
		if (IsValid(RerollCostText))
		{
			RerollCostText->SetText(FText::GetEmpty());
		}
		if (IsValid(RerollButton))
		{
			RerollButton->SetIsEnabled(false);
		}
		if (IsValid(UpgradePreviewText))
		{
			UpgradePreviewText->SetText(FText::GetEmpty());
		}
		if (IsValid(UpgradeChanceText))
		{
			UpgradeChanceText->SetText(FText::GetEmpty());
		}
		if (IsValid(UpgradeCostText))
		{
			UpgradeCostText->SetText(FText::GetEmpty());
		}
		if (IsValid(UpgradeButton))
		{
			UpgradeButton->SetIsEnabled(false);
		}
		return;
	}

	const FPrimaryAssetId DefId = Def->GetPrimaryAssetId();
	const FNSPartDefinitionRow* Row = NSPartUtils::ResolvePartRow(this, DefId);
	const int64 Balance = Currency ? Currency->GetTemp() : 0;

	// ---- 리롤 박스 ----
	const int64 RerollCost = EquipComp->GetRerollCost(SelectedUpgradeSlot);
	if (IsValid(RerollRangeText))
	{
		const FNSPartUpgradeRow* UpgradeRow = NSPartUtils::ResolvePartUpgradeRow(this, Part->CurrentRarity);
		RerollRangeText->SetText(UpgradeRow
			? FText::Format(NSLOCTEXT("PartUpgrade", "RerollRange", "스텟 변동폭 : {0} ~ {1}"),
				FText::AsNumber(UpgradeRow->ValueRange.Min), FText::AsNumber(UpgradeRow->ValueRange.Max))
			: FText::GetEmpty());
	}
	if (IsValid(RerollCostText))
	{
		RerollCostText->SetText(RerollCost >= 0
			? FText::Format(NSLOCTEXT("PartUpgrade", "RerollCost", "리롤 비용 : {0}"), FText::AsNumber(RerollCost))
			: FText::GetEmpty());
	}
	if (IsValid(RerollButton))
	{
		const bool bCanReroll = Row && Row->bCanReroll && RerollCost >= 0 && Balance >= RerollCost;
		RerollButton->SetIsEnabled(bCanReroll);
	}

	// ---- 업그레이드 박스 ----
	const int64 UpgradeCost = EquipComp->GetUpgradeCost(SelectedUpgradeSlot);
	const float UpgradeChance = EquipComp->GetUpgradeChance(SelectedUpgradeSlot);
	const bool bIsLegendary = Part->CurrentRarity == ENSPartRarity::Legendary;

	if (IsValid(UpgradePreviewText))
	{
		if (bIsLegendary)
		{
			UpgradePreviewText->SetText(NSLOCTEXT("PartUpgrade", "MaxRarity", "최고 등급"));
		}
		else
		{
			const ENSPartRarity NextRarity =
				static_cast<ENSPartRarity>(static_cast<uint8>(Part->CurrentRarity) + 1);
			const FNSPartUpgradeRow* NextUpgradeRow = NSPartUtils::ResolvePartUpgradeRow(this, NextRarity);
			const FText RarityChange = FText::Format(NSLOCTEXT("PartUpgrade", "RarityChange", "변동 : {0} → {1}"),
				GetRarityDisplayText(Part->CurrentRarity), GetRarityDisplayText(NextRarity));
			const FText ValueChange = NextUpgradeRow
				? FText::Format(NSLOCTEXT("PartUpgrade", "ValueChange", "스텟 : {0} → {1} ~ {2}"),
					FText::AsNumber(Part->CurrentValue), FText::AsNumber(NextUpgradeRow->ValueRange.Min), FText::AsNumber(NextUpgradeRow->ValueRange.Max))
				: FText::GetEmpty();
			UpgradePreviewText->SetText(FText::Format(
				NSLOCTEXT("PartUpgrade", "UpgradePreview", "{0}\n{1}"), RarityChange, ValueChange));
		}
	}
	if (IsValid(UpgradeChanceText))
	{
		UpgradeChanceText->SetText(UpgradeChance >= 0.f
			? FText::Format(NSLOCTEXT("PartUpgrade", "UpgradeChance", "확률 : {0}"), FText::AsPercent(UpgradeChance))
			: FText::GetEmpty());
	}
	if (IsValid(UpgradeCostText))
	{
		UpgradeCostText->SetText(UpgradeCost >= 0
			? FText::Format(NSLOCTEXT("PartUpgrade", "UpgradeCost", "업그레이드 비용 : {0}"), FText::AsNumber(UpgradeCost))
			: FText::GetEmpty());
	}
	if (IsValid(UpgradeButton))
	{
		UpgradeButton->SetIsEnabled(!bIsLegendary && UpgradeCost >= 0 && Balance >= UpgradeCost);
	}
}

// ================================================================
// 델리게이트 핸들러
// ================================================================

void UNSPartUpgradeWidget::HandlePartChanged(FGameplayTag PartSlot, const FNSPartData& Part)
{
	RefreshEquippedDisplays();
	if (PartSlot == SelectedUpgradeSlot)
	{
		RefreshUpgradePanels();
		RefreshSelectedSlotPreview();
	}
}

void UNSPartUpgradeWidget::HandleTempChanged(int64 NewAmount)
{
	RefreshBalance();
	RefreshBuyBox();
	RefreshUpgradePanels();
}

void UNSPartUpgradeWidget::HandleUpgradeResult(FGameplayTag PartSlot, ENSPartUpgradeResult Result, int64 NewTempBalance)
{
	// Wallet 프로퍼티 복제를 기다리지 않고 결과와 함께 온 잔액으로 즉시 갱신
	SetBalanceText(NewTempBalance);
	RefreshBuyBox();
	RefreshUpgradePanels();

	OnUpgradeResultReceived(PartSlot, Result);
}

void UNSPartUpgradeWidget::HandleShopStockChanged()
{
	RefreshStockEntries();
}

// ================================================================
// 클릭 핸들러
// ================================================================

void UNSPartUpgradeWidget::OnStockEntryClicked(const FNSPartDefinitionRow& Row, UNSPartCatalogEntryWidget* Entry, int32 StockIndex)
{
	SelectedStockIndex = StockIndex;

	for (UNSPartCatalogEntryWidget* Widget : StockEntryWidgets)
	{
		if (IsValid(Widget))
		{
			Widget->SetIsSelected(Widget == Entry);
		}
	}

	const UNSPartEquipComponent* EquipComp = GetEquipComponent();
	const TArray<FNSPartData>* Stock = EquipComp ? &EquipComp->GetShopStock() : nullptr;
	if (Stock && Stock->IsValidIndex(StockIndex))
	{
		const FNSPartData& Item = (*Stock)[StockIndex];
		UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, Item);
		if (IsValid(StockDetailWidget))
		{
			StockDetailWidget->SetupFromInstance(Item, Def);
		}
		UpdatePreview(Def, StockDetailWidget);
	}

	RefreshBuyBox();
}

void UNSPartUpgradeWidget::OnUpgradeBodyClicked()
{
	SelectUpgradeSlot(BodySlotTag);
}

void UNSPartUpgradeWidget::OnUpgradeArmClicked()
{
	SelectUpgradeSlot(ArmSlotTag);
}

void UNSPartUpgradeWidget::OnUpgradeLegClicked()
{
	SelectUpgradeSlot(LegSlotTag);
}

void UNSPartUpgradeWidget::SelectUpgradeSlot(FGameplayTag SlotTag)
{
	SelectedUpgradeSlot = SlotTag;
	RefreshUpgradePanels();
	RefreshSelectedSlotPreview();
}

void UNSPartUpgradeWidget::OnCloseClicked()
{
	CloseWidget();
}

void UNSPartUpgradeWidget::OnHubPurchaseClicked()
{
	OpenPurchasePage();
}

void UNSPartUpgradeWidget::OnHubUpgradeClicked()
{
	OpenUpgradePage();
}

void UNSPartUpgradeWidget::OnBackClicked()
{
	OpenHubPage();
}

void UNSPartUpgradeWidget::OnBuyClicked()
{
	UNSPartEquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp || SelectedStockIndex == INDEX_NONE)
	{
		return;
	}
	EquipComp->Server_RequestPurchase(SelectedStockIndex);
	SelectedStockIndex = INDEX_NONE;
	RefreshBuyBox();
}

void UNSPartUpgradeWidget::OnRerollClicked()
{
	UNSPartEquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp || !SelectedUpgradeSlot.IsValid())
	{
		return;
	}
	EquipComp->Server_RequestReroll(SelectedUpgradeSlot);
}

void UNSPartUpgradeWidget::OnUpgradeClicked()
{
	UNSPartEquipComponent* EquipComp = GetEquipComponent();
	if (!EquipComp || !SelectedUpgradeSlot.IsValid())
	{
		return;
	}
	EquipComp->Server_RequestUpgradeRarity(SelectedUpgradeSlot);
}
