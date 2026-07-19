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
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/UI/HUD/NSCharacterStatsBridgeSubsystem.h"
#include "NeoSanctum/Progression/Part/NSPartPreviewStage.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/UI/Part/Button/NSPartSlotButton.h"
#include "NeoSanctum/UI/Part/NSPartCatalogEntryWidget.h"
#include "NeoSanctum/UI/Part/NSPartDetailWidget.h"
#include "NeoSanctum/UI/Common/NSNoticePopupWidget.h"

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
		// 소수점은 버리고 정수로만 표시. 반올림이면 3.8이 4로 보여 실제보다 좋아 보이므로 내림 고정
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		Options.MinimumFractionalDigits = 0;
		Options.RoundingMode = ERoundingMode::ToNegativeInfinity;
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
	if (IsValid(UpgradeCloseButton))
	{
		UpgradeCloseButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnCloseClicked);
	}
	if (IsValid(HubCloseButton))
	{
		HubCloseButton->OnClicked.AddUniqueDynamic(this, &UNSPartUpgradeWidget::OnCloseClicked);
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

	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
		GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->BroadcastCharacterStats(Interactor);
	}

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

	// SetFocus()는 Is Focusable이 꺼져 있으면 조용히 실패해 ESC(NativeOnKeyDown)를 아예 못 받으므로 반드시 켠다
	SetIsFocusable(true);

	// 게임 키보드 입력을 완전히 차단하고 마우스만 받도록 UIOnly로 전환.
	// UIOnly는 ANSPlayerController의 네이티브 Escape 바인딩도 막으므로 NativeOnKeyDown에서 직접 처리한다.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Interactor->SetInputMode(InputMode);

	// 버튼 클릭 등으로 포커스가 이동해도 이 위젯이 ESC를 계속 받도록 보장
	SetFocus();
}

FReply UNSPartUpgradeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseWidget();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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

	// X버튼/ESC 등 위젯 자체 경로로 닫혀도 이동 매핑 복원 + ActiveInteractionWidget 정리가 되도록 통지
	if (ANSPlayerController* NSPC = Cast<ANSPlayerController>(OwningController.Get()))
	{
		NSPC->NotifyInteractionWidgetClosed(this);
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

	// 리롤/등급업 프리뷰에 표시할 수치 범위는 이 파츠 인스턴스 스탯의 등급별 범위 DT가 단일 소스
	const FGameplayTag PartStatTag = NSPartUtils::GetPartStatTag(this, *Part);

	// ---- 리롤 박스 ----
	const int64 RerollCost = EquipComp->GetRerollCost(SelectedUpgradeSlot);
	if (IsValid(RerollRangeText))
	{
		FNSPartValueRange RerollRange;
		const bool bHasRerollRange = NSPartUtils::GetStatValueRange(this, PartStatTag, Part->CurrentRarity, RerollRange);
		RerollRangeText->SetText(bHasRerollRange
			? FText::Format(NSLOCTEXT("PartUpgrade", "RerollRange", "스텟 변동폭 : {0} ~ {1}"),
				FormatSummaryValue(RerollRange.Min), FormatSummaryValue(RerollRange.Max))
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
			const FText RarityChange = FText::Format(NSLOCTEXT("PartUpgrade", "RarityChange", "변동 : {0} → {1}"),
				GetRarityDisplayText(Part->CurrentRarity), GetRarityDisplayText(NextRarity));
			// 다음 등급의 수치 범위를 그대로 표시 (범위 없음 = 다음 등급에 이 스탯이 정의 안 됨 → 수치 줄 생략)
			FNSPartValueRange NextRange;
			const bool bHasNextRange = NSPartUtils::GetStatValueRange(this, PartStatTag, NextRarity, NextRange);
			const FText ValueChange = bHasNextRange
				? FText::Format(NSLOCTEXT("PartUpgrade", "ValueChange", "스텟 : {0} → {1} ~ {2}"),
					FormatSummaryValue(Part->CurrentValue), FormatSummaryValue(NextRange.Min), FormatSummaryValue(NextRange.Max))
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

	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
		GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->BroadcastCharacterStats(OwningController.Get());
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
			// SetIsSelected(false)는 bToggleable이 꺼진 버튼에서는 무시되므로, 해제는 ClearSelection() 사용
			if (Widget == Entry)
			{
				Widget->SetIsSelected(true);
			}
			else
			{
				Widget->ClearSelection();
			}
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
