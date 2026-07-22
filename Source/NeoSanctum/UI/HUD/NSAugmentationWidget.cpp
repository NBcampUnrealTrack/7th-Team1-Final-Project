// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentationWidget.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/UI/HUD/NSAugmentCardWidget.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/HUD/NSAugmentDisplayBridgeSubsystem.h"
#include "NeoSanctum/UI/HUD/NSCharacterStatsBridgeSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WrapBox.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "TimerManager.h"


void UNSAugmentationWidget::OpenPanel()
{
	// 전체 증강 패널 표시
	bPanelOpen = true;
	SetVisibility(ESlateVisibility::Visible);
	SetOwnedAugmentListVisible(true);
	RefreshOwnedAugmentList();
	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::ClosePanel()
{
	bPanelOpen = false;

	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (OwnedIconLoadHandle.IsValid())
	{
		OwnedIconLoadHandle->CancelHandle();
		OwnedIconLoadHandle.Reset();
	}

	SetOwnedAugmentListVisible(false);
	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::ShowCardSection()
{
	if (CardSectionRoot)
	{
		CardSectionRoot->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshChoiceGuideVisibility();
}

void UNSAugmentationWidget::HideCardSection()
{
	ClearSelectionAnimationTimer();

	ClearChoiceCardWidgets();

	CurrentOfferCards.Reset();
	CurrentOfferViewData.Reset();
	CurrentCardsPerSlot.Reset();

	ChoiceCount = 0;
	ChoiceGuideSlotCount = 0;
	CurrentAvailableCardCount = 0;
	HighlightedCardIndex = INDEX_NONE;

	if (CardSectionRoot)
	{
		CardSectionRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshChoiceGuideVisibility();
}

void UNSAugmentationWidget::CreateChoiceCard(int32 NewChoiceCount)
{
	if (!ChoiceRootCanvas)
	{
		return;
	}

	ClearSelectionAnimationTimer();
	ClearChoiceCardWidgets();

	ChoiceCount = FMath::Clamp(NewChoiceCount, 0, ChoiceGuideSlotCount);

	if (!AugmentCardWidgetClass || ChoiceCount <= 0)
	{
		ChoiceCount = 0;
		RefreshChoiceGuideVisibility();
		return;
	}

	for (int32 Index = 0; Index < ChoiceCount; ++Index)
	{
		UNSAugmentCardWidget* NewCard =
			CreateWidget<UNSAugmentCardWidget>(this, AugmentCardWidgetClass);

		if (!NewCard)
		{
			continue;
		}

		NewCard->ResetSelectionVisual();
		AugmentCardWidgets.Add(NewCard);

		NewCard->SetAugmentName(TEXT("Loading"));
		NewCard->SetAugmentDescription(TEXT("Waiting ViewData"));
		NewCard->SetAugmentIcon(nullptr);

		UCanvasPanelSlot* CardSlot = ChoiceRootCanvas->AddChildToCanvas(NewCard);
		if (CardSlot)
		{
			CardSlot->SetAutoSize(false);
			CardSlot->SetSize(ChoiceCardSize);
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardSlot->SetZOrder(1000 - Index);
		}
	}

	RefreshChoiceGuideVisibility();
	QueueChoiceCardPositionRefresh();
}

void UNSAugmentationWidget::RefreshChoiceGuideVisibility()
{
	// 선택 애니메이션 중에는 선택 가이드를 숨김
	const bool bSelectionGuideVisible =
		bPanelOpen &&
		!CurrentOfferCards.IsEmpty() &&
		ChoiceCount > 0 &&
		!bSelectionAnimationPlaying;

	if (ChoiceGuide3Root)
	{
		ChoiceGuide3Root->SetVisibility(
			bSelectionGuideVisible && ChoiceGuideSlotCount == 3
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (ChoiceGuide4Root)
	{
		ChoiceGuide4Root->SetVisibility(
			bSelectionGuideVisible && ChoiceGuideSlotCount == 4
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	RefreshChoiceGuideSlotVisibility();
}

void UNSAugmentationWidget::RefreshChoiceCardPositions()
{
	ForceLayoutPrepass();

	for (int32 Index = 0;
	     Index < AugmentCardWidgets.Num();
	     ++Index)
	{
		UNSAugmentCardWidget* Card =
			AugmentCardWidgets[Index];

		if (!Card)
		{
			continue;
		}

		UCanvasPanelSlot* CardSlot =
			Cast<UCanvasPanelSlot>(Card->Slot);

		if (!CardSlot)
		{
			continue;
		}

		CardSlot->SetAutoSize(false);
		CardSlot->SetSize(ChoiceCardSize);
		CardSlot->SetAlignment(
			FVector2D(0.5f, 0.5f));

		CardSlot->SetPosition(
			GetChoiceCardPosition(Index));
	}
}

FVector2D UNSAugmentationWidget::GetChoiceCardPosition(int32 Index) const
{
	return GetChoiceCardPositionBySlotIndex(
		GetChoiceGuideSlotIndexForCardIndex(Index));
}

bool UNSAugmentationWidget::TryGetWidgetCanvasRect(
	const UWidget* Widget,
	FVector2D& OutTopLeft,
	FVector2D& OutSize) const
{
	OutTopLeft = FVector2D::ZeroVector;
	OutSize = FVector2D::ZeroVector;

	if (!Widget || !ChoiceRootCanvas)
	{
		return false;
	}

	const UWidget* RootCanvasWidget = ChoiceRootCanvas.Get();

	const FGeometry WidgetGeometry = Widget->GetCachedGeometry();
	const FGeometry RootCanvasGeometry = RootCanvasWidget->GetCachedGeometry();

	FVector2D LocalSize = WidgetGeometry.GetLocalSize();

	if (LocalSize.IsNearlyZero())
	{
		LocalSize = Widget->GetDesiredSize();
	}

	if (LocalSize.IsNearlyZero())
	{
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			LocalSize = CanvasSlot->GetSize();
		}
	}

	if (LocalSize.IsNearlyZero())
	{
		return false;
	}

	const FVector2D AbsoluteTopLeft = WidgetGeometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D RootLocalTopLeft = RootCanvasGeometry.AbsoluteToLocal(AbsoluteTopLeft);

	OutTopLeft = RootLocalTopLeft;
	OutSize = LocalSize;

	return true;
}

int32 UNSAugmentationWidget::GetSelectableChoiceCount() const
{
	return FMath::Clamp(CurrentOfferCards.Num(), 0, FMath::Max(ChoiceGuideSlotCount, 0));
}

USizeBox* UNSAugmentationWidget::GetChoiceGuideInputIconBoxBySlotIndex(int32 SlotIndex) const
{
	if (ChoiceGuideSlotCount == 4)
	{
		switch (SlotIndex)
		{
		case 0: return Choice4InputIcon1Box.Get();
		case 1: return Choice4InputIcon2Box.Get();
		case 2: return Choice4InputIcon3Box.Get();
		case 3: return Choice4InputIcon4Box.Get();
		default: return nullptr;
		}
	}

	if (ChoiceGuideSlotCount == 3)
	{
		switch (SlotIndex)
		{
		case 0: return Choice3InputIcon1Box.Get();
		case 1: return Choice3InputIcon2Box.Get();
		case 2: return Choice3InputIcon3Box.Get();
		default: return nullptr;
		}
	}

	return nullptr;
}

USizeBox* UNSAugmentationWidget::GetChoiceGuideInputIconBox(int32 CardIndex) const
{
	return GetChoiceGuideInputIconBoxBySlotIndex(
		GetChoiceGuideSlotIndexForCardIndex(CardIndex));
}

FVector2D UNSAugmentationWidget::GetChoiceCardPositionBySlotIndex(int32 SlotIndex) const
{
	const USizeBox* IconBox = GetChoiceGuideInputIconBoxBySlotIndex(SlotIndex);
	if (!IconBox)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D IconTopLeft = FVector2D::ZeroVector;
	FVector2D IconSize = FVector2D::ZeroVector;

	if (!TryGetWidgetCanvasRect(IconBox, IconTopLeft, IconSize))
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D IconCenter = IconTopLeft + IconSize * 0.5f;

	if (ChoiceGuideSlotCount == 3)
	{
		switch (SlotIndex)
		{
		case 0:
			return FVector2D(
				IconTopLeft.X - ChoiceCardGuideGap - ChoiceCardSize.X * 0.5f,
				IconCenter.Y);

		case 1:
			return FVector2D(
				IconCenter.X,
				IconTopLeft.Y - ChoiceCardGuideGap - ChoiceCardSize.Y * 0.5f);

		case 2:
			return FVector2D(
				IconTopLeft.X + IconSize.X + ChoiceCardGuideGap + ChoiceCardSize.X * 0.5f,
				IconCenter.Y);

		default:
			return FVector2D::ZeroVector;
		}
	}

	if (ChoiceGuideSlotCount == 4)
	{
		switch (SlotIndex)
		{
		case 0:
		case 1:
			return FVector2D(
				IconTopLeft.X - ChoiceCardGuideGap - ChoiceCardSize.X * 0.5f,
				IconCenter.Y);

		case 2:
		case 3:
			return FVector2D(
				IconTopLeft.X + IconSize.X + ChoiceCardGuideGap + ChoiceCardSize.X * 0.5f,
				IconCenter.Y);

		default:
			return FVector2D::ZeroVector;
		}
	}

	return FVector2D::ZeroVector;
}

void UNSAugmentationWidget::ClearChoiceCardWidgets()
{
	for (UNSAugmentCardWidget* ExistingCard :
	     AugmentCardWidgets)
	{
		if (ExistingCard)
		{
			ExistingCard->RemoveFromParent();
		}
	}

	AugmentCardWidgets.Reset();
}

int32 UNSAugmentationWidget::GetChoiceGuideSlotIndexForCardIndex(
	int32 CardIndex) const
{
	const int32 SelectableChoiceCount =
		GetSelectableChoiceCount();

	if (CardIndex < 0 ||
		CardIndex >= SelectableChoiceCount ||
		CardIndex >= ChoiceGuideSlotCount)
	{
		return INDEX_NONE;
	}

	return CardIndex;
}

TArray<int32>
UNSAugmentationWidget::CalculateCardsPerSlot(
	int32 TotalRemainingCardCount) const
{
	const int32 SlotCount =
		FMath::Clamp(
			ChoiceGuideSlotCount,
			0,
			4);

	TArray<int32> CardsPerSlot;
	CardsPerSlot.Init(
		0,
		SlotCount);

	if (SlotCount <= 0 ||
		TotalRemainingCardCount <= 0)
	{
		return CardsPerSlot;
	}

	const int32 SafeRemainingCardCount =
		FMath::Max(
			TotalRemainingCardCount,
			0);

	const int32 BaseCount =
		SafeRemainingCardCount /
		SlotCount;

	const int32 Remainder =
		SafeRemainingCardCount %
		SlotCount;

	for (int32 SlotIndex = 0;
	     SlotIndex < SlotCount;
	     ++SlotIndex)
	{
		// 나머지는 1번, 2번, 3번, 4번 순으로 배치합니다.
		CardsPerSlot[SlotIndex] =
			BaseCount +
			(SlotIndex < Remainder ? 1 : 0);
	}

	return CardsPerSlot;
}

bool UNSAugmentationWidget::IsChoiceGuideSlotActive(
	int32 SlotIndex) const
{
	return
		SlotIndex >= 0 &&
		SlotIndex < ChoiceGuideSlotCount &&
		CurrentCardsPerSlot.IsValidIndex(
			SlotIndex) &&
		CurrentCardsPerSlot[SlotIndex] > 0;
}

void UNSAugmentationWidget::QueueChoiceCardPositionRefresh()
{
	if (!ChoiceRootCanvas)
	{
		return;
	}

	const bool bSelectionOpen = bPanelOpen && !CurrentOfferCards.IsEmpty();

	// 선택 패널이 아직 열리지 않은 상태에서는 가이드 Geometry가 유효하지 않을 수 있습니다.
	// Tab으로 선택 패널이 열린 뒤 RefreshAugmentPanelState에서 다시 예약됩니다.
	if (!bSelectionOpen)
	{
		ChoiceRootCanvas->SetRenderOpacity(1.f);
		return;
	}

	if (bChoiceCardPositionRefreshQueued)
	{
		return;
	}

	bChoiceCardPositionRefreshQueued = true;

	// 첫 프레임에 잘못된 좌표가 잠깐 보이는 것을 막습니다.
	ChoiceRootCanvas->SetRenderOpacity(0.f);

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate Delegate;
		Delegate.BindUObject(
			this,
			&UNSAugmentationWidget::HandleDeferredChoiceCardPositionRefresh);

		World->GetTimerManager().SetTimerForNextTick(Delegate);
		return;
	}

	HandleDeferredChoiceCardPositionRefresh();
}

void UNSAugmentationWidget::HandleDeferredChoiceCardPositionRefresh()
{
	bChoiceCardPositionRefreshQueued = false;

	const bool bSelectionOpen =
		bPanelOpen &&
		!CurrentOfferCards.IsEmpty();

	if (!bSelectionOpen)
	{
		if (ChoiceRootCanvas)
		{
			ChoiceRootCanvas->SetRenderOpacity(1.f);
		}
		return;
	}

	RefreshChoiceGuideVisibility();
	ForceLayoutPrepass();
	RefreshChoiceCardPositions();

	if (ChoiceRootCanvas)
	{
		ChoiceRootCanvas->SetRenderOpacity(1.f);
	}
}

void UNSAugmentationWidget::SelectCardByIndex(int32 CardIndex)
{
	// 선택 애니메이션 중에는 추가 선택 입력을 무시
	if (bSelectionAnimationPlaying)
	{
		return;
	}

	// 유효하지 않은 카드 번호는 무시
	if (!CurrentOfferCards.IsValidIndex(CardIndex) ||
		!AugmentCardWidgets.IsValidIndex(CardIndex))
	{
		return;
	}

	// 서버 선택 요청 전에 카드 선택 연출을 시작
	PlayAugmentSelectSound();
	BeginCardSelection(CardIndex);
}

void UNSAugmentationWidget::ConfirmAugmentSelection(int32 CardIndex)
{
	// 리롤 응답을 기다리는 중이면 카드 선택을 막음
	if (bRerollRequestPending)
	{
		return;
	}

	// 현재 오퍼 범위 밖이면 무시
	if (!CurrentOfferCards.IsValidIndex(CardIndex))
	{
		return;
	}

	UNSAugmentSelectionComponent* SelComp = GetSelectionComponent();
	if (!SelComp)
	{
		return;
	}

	// 서버 권한에서 증강 적용. UI 숨김은 서버의 Client_CloseOffer -> OnOfferClosed가 처리
	SelComp->Server_Choose(CardIndex, CurrentOfferRevision);
}

void UNSAugmentationWidget::RequestRerollAugment()
{
	// 선택 애니메이션 중이거나 리롤 불가 상태면 리롤 요청을 막음
	if (bSelectionAnimationPlaying ||
		bRerollRequestPending ||
		!bCanRerollCurrentOffer ||
		CurrentOfferCards.IsEmpty())
	{
		return;
	}

	UNSAugmentSelectionComponent* SelComp = GetSelectionComponent();
	if (!SelComp)
	{
		return;
	}

	bRerollRequestPending = true;
	SetRerollStatusMessage(FText::FromString(TEXT("리롤 중입니다.")));
	RefreshRerollControls();

	// 서버에 전체 리롤 요청 -> Client_PresentOffer -> HandleOfferPresented로 카드 갱신
	SelComp->Server_RerollCard(CurrentOfferRevision);
}

bool UNSAugmentationWidget::CanAffordReroll()
{
	const UNSCurrencyComponent* CurrencyComp = GetCurrencyComponent();
	return CurrencyComp && CurrencyComp->GetTemp() >= CurrentRerollCost;
}

bool UNSAugmentationWidget::IsRerollAvailable()
{
	// 선택 애니메이션 중에는 리롤을 비활성화
	return
		!bSelectionAnimationPlaying &&
		!bRerollRequestPending &&
		bCanRerollCurrentOffer &&
		CanAffordReroll();
}

void UNSAugmentationWidget::RefreshRerollControls()
{
	const bool bSelectionOpen = bPanelOpen && !CurrentOfferCards.IsEmpty();

	// 선택 애니메이션 중에는 리롤 UI를 숨김
	const bool bShouldShowReroll =
		bSelectionOpen &&
		bCanRerollCurrentOffer &&
		!bSelectionAnimationPlaying;

	const ESlateVisibility ButtonVisibility =
		bShouldShowReroll
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed;

	const ESlateVisibility DisplayVisibility =
		bShouldShowReroll
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed;

	if (RerollButton)
	{
		RerollButton->SetVisibility(ButtonVisibility);
	}

	if (RerollInputIcon)
	{
		RerollInputIcon->SetVisibility(DisplayVisibility);
	}

	if (RerollCostText)
	{
		RerollCostText->SetVisibility(DisplayVisibility);
		RerollCostText->SetText(FText::AsNumber(CurrentRerollCost));
	}
}

void UNSAugmentationWidget::SetRerollStatusMessage(const FText& Message)
{
	if (!RerollStatusText)
	{
		return;
	}

	RerollStatusText->SetText(Message);
	RerollStatusText->SetVisibility(
		Message.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
}

void UNSAugmentationWidget::RefreshOwnedAugmentList()
{
	if (!AreOwnedAugmentListReady())
	{
		return;
	}

	ClearOwnedAugmentLists();
	RefreshOwnedAugmentSectionVisibility();

	UNSAugmentInventoryComponent* Inv = GetInventoryComponent();
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Inv || !Data || !Data->IsRunReady())
	{
		return;
	}

	// 보유 증강 아이콘 소프트 오브젝트 경로 수집
	TArray<FSoftObjectPath> PathsToLoad;
	for (const FNSAugmentInstance& Inst : Inv->GetOwned())
	{
		const UNSAugmentDefinition* Def =
			Data->GetData<UNSAugmentDefinition>(Inst.DefId);

		if (Def && !Def->Icon.IsNull())
		{
			PathsToLoad.Add(Def->Icon.ToSoftObjectPath());
		}
	}

	if (PathsToLoad.Num() == 0)
	{
		return;
	}

	if (OwnedIconLoadHandle.IsValid())
	{
		OwnedIconLoadHandle->CancelHandle();
		OwnedIconLoadHandle.Reset();
	}

	OwnedIconLoadHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			PathsToLoad,
			FStreamableDelegate::CreateUObject(
				this,
				&UNSAugmentationWidget::OnOwnedIconsLoaded));
}

void UNSAugmentationWidget::OnOwnedIconsLoaded()
{
	if (!AreOwnedAugmentListReady() || !WidgetTree)
	{
		return;
	}

	ClearOwnedAugmentLists();
	RefreshOwnedAugmentSectionVisibility();

	UNSAugmentInventoryComponent* Inv = GetInventoryComponent();
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Inv || !Data)
	{
		return;
	}

	const UObject* RiaSansFontObject = LoadObject<UObject>(
		nullptr,
		TEXT("/Game/NeoSanctum/UI/Asset/Font/CF_RiaSans.CF_RiaSans"));

	for (const FNSAugmentInstance& Inst : Inv->GetOwned())
	{
		const UNSAugmentDefinition* Def =
			Data->GetData<UNSAugmentDefinition>(Inst.DefId);

		if (!Def)
		{
			continue;
		}

		UTexture2D* Texture = Def->Icon.Get();
		if (!Texture)
		{
			continue;
		}

		const int32 StackCount = Inv->GetStackCount(Inst.DefId);

		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());

		if (!SizeBox || !Overlay || !ScaleBox || !IconImage)
		{
			continue;
		}

		SizeBox->SetWidthOverride(OwnedIconSize.X);
		SizeBox->SetHeightOverride(OwnedIconSize.Y);

		ScaleBox->SetStretch(EStretch::ScaleToFit);
		ScaleBox->SetStretchDirection(EStretchDirection::Both);

		IconImage->SetBrushFromTexture(Texture, true);

		ScaleBox->AddChild(IconImage);

		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(ScaleBox);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
		}

		if (StackCount > 1)
		{
			UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

			if (CountText)
			{
				FSlateFontInfo FontInfo = CountText->GetFont();
				if (RiaSansFontObject)
				{
					FontInfo.FontObject = RiaSansFontObject;
				}
				FontInfo.Size = 12;
				FontInfo.OutlineSettings.OutlineSize = 1;
				FontInfo.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.03f, 0.08f, 1.f);
				CountText->SetFont(FontInfo);

				CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				CountText->SetText(FText::Format(
					NSLOCTEXT("AugmentationWidget", "OwnedAugmentCount", "{0}"),
					FText::AsNumber(StackCount)));

				UOverlaySlot* BadgeSlot = Overlay->AddChildToOverlay(CountText);

				if (BadgeSlot)
				{
					BadgeSlot->SetHorizontalAlignment(HAlign_Right);
					BadgeSlot->SetVerticalAlignment(VAlign_Top);
					BadgeSlot->SetPadding(FMargin(0.f));
				}
			}
		}

		UWrapBox* TargetWrapBox = GetOwnedAugmentWrapBox(Inst.Rarity);
		if (!TargetWrapBox)
		{
			continue;
		}

		SizeBox->AddChild(Overlay);
		TargetWrapBox->AddChildToWrapBox(SizeBox);
	}

	RefreshOwnedAugmentSectionVisibility();
}

void UNSAugmentationWidget::HighLightCard(int32 CardIndex)
{
	if (!AugmentCardWidgets.IsValidIndex(CardIndex))
	{
		return;
	}

	HighlightedCardIndex = CardIndex;

	for (int32 Index = 0; Index < AugmentCardWidgets.Num(); ++Index)
	{
		if (!AugmentCardWidgets[Index])
		{
			continue;
		}

		AugmentCardWidgets[Index]->SetHighLighted(Index == HighlightedCardIndex);
	}
}

void UNSAugmentationWidget::RefreshAugmentPanelState()
{
	const UNSAugmentSelectionComponent* SelectionComp = SelectionComponent.Get();

	const int32 PendingCount = SelectionComp
		                           ? SelectionComp->GetPendingCount()
		                           : 0;

	const bool bHasPendingAugment = PendingCount > 0;
	const bool bHasOfferCards = !CurrentOfferCards.IsEmpty();

	const bool bHasAugmentPrompt = bHasPendingAugment || bHasOfferCards;

	// 선택 카드 캔버스는 선택 애니메이션 중에도 계속 표시
	const bool bChoiceCardsVisible =
		bPanelOpen &&
		bHasOfferCards;

	// 선택 가이드와 리롤 UI는 선택 애니메이션 중에 숨김
	const bool bSelectionControlsVisible =
		bChoiceCardsVisible &&
		!bSelectionAnimationPlaying;

	const bool bShouldShowAugmentWidget =
		bPanelOpen ||
		bOwnedListRequested ||
		bHasAugmentPrompt;

	if (!bShouldShowAugmentWidget)
	{
		SetVisibility(ESlateVisibility::Collapsed);

		if (CardSectionRoot)
		{
			CardSectionRoot->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (ChoiceRootCanvas)
		{
			ChoiceRootCanvas->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (CenterControlRoot)
		{
			CenterControlRoot->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (TabButton)
		{
			TabButton->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (RemainingAugmentCountText)
		{
			RemainingAugmentCountText->SetVisibility(ESlateVisibility::Collapsed);
		}

		RefreshRerollControls();
		RefreshChoiceGuideVisibility();
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	if (CardSectionRoot)
	{
		CardSectionRoot->SetVisibility(
			bHasAugmentPrompt
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (ChoiceRootCanvas)
	{
		ChoiceRootCanvas->SetVisibility(
			bChoiceCardsVisible
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (CenterControlRoot)
	{
		CenterControlRoot->SetVisibility(
			bHasAugmentPrompt
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (TabButton)
	{
		TabButton->SetVisibility(
			bHasAugmentPrompt && !bChoiceCardsVisible
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (RemainingAugmentCountText)
	{
		RemainingAugmentCountText->SetVisibility(
			bHasAugmentPrompt
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);

		RemainingAugmentCountText->SetText(FText::AsNumber(PendingCount));
	}

	RefreshRerollControls();
	RefreshChoiceGuideVisibility();

	if (bSelectionControlsVisible)
	{
		QueueChoiceCardPositionRefresh();
	}
}

void UNSAugmentationWidget::SetOwnedAugmentListVisible(bool bVisible)
{
	bOwnedListRequested = bVisible;

	if (OwnedAugmentPanelRoot)
	{
		OwnedAugmentPanelRoot->SetVisibility(
			bVisible
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::OpenSelectionPanel()
{
	bPanelOpen = true;

	SetVisibility(ESlateVisibility::Visible);
	SetOwnedAugmentListVisible(false);
	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::PlayAugmentSelectSound() const
{
	PlayAugmentSound(AugmentSelectSoundID);
}

void UNSAugmentationWidget::PlayAugmentRerollSuccessSound() const
{
	PlayAugmentSound(AugmentRerollSuccessSoundID);
}

void UNSAugmentationWidget::PlayAugmentRerollFailSound() const
{
	PlayAugmentSound(AugmentRerollFailSoundID);
}

void UNSAugmentationWidget::PlayAugmentTabSound() const
{
	PlayAugmentSound(AugmentTabSoundID);
}

void UNSAugmentationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 기본상태에서는 패널 숨김 + 카드 영역 숨김
	bPanelOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	HideCardSection();
	SetOwnedAugmentListVisible(false);

	// 오너 PC의 선택 컴포넌트 델리게이트 구독
	if (UNSAugmentSelectionComponent* SelComp = GetSelectionComponent())
	{
		SelComp->OnOfferPresented.AddDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelComp->OnOfferClosed.AddDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		SelComp->OnPendingCountChanged.AddDynamic(this, &UNSAugmentationWidget::HandlePendingCountChanged);
		SelComp->OnRerollResult.AddDynamic(this, &UNSAugmentationWidget::HandleRerollResult);

		// 현재 대기 카운트로 뱃지 초기화
		HandlePendingCountChanged(SelComp->GetPendingCount());
	}

	// 보유 증강 변경 구독
	if (UNSAugmentInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->OnInventoryChanged.AddDynamic(this, &UNSAugmentationWidget::HandleInventoryChanged);
	}

	if (RerollButton)
	{
		RerollButton->OnClicked.AddDynamic(this, &UNSAugmentationWidget::RequestRerollAugment);
	}

	RefreshRerollControls();
	SetRerollStatusMessage(FText::GetEmpty());
	RefreshChoiceGuideVisibility();
}

void UNSAugmentationWidget::NativeDestruct()
{
	// 위젯 제거 전에 선택 애니메이션 타이머를 정리
	ClearSelectionAnimationTimer();

	// 진행 중인 비동기 로드 취소
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (OwnedIconLoadHandle.IsValid())
	{
		OwnedIconLoadHandle->CancelHandle();
		OwnedIconLoadHandle.Reset();
	}

	// 구독 해제
	if (SelectionComponent.IsValid())
	{
		SelectionComponent->OnOfferPresented.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelectionComponent->OnOfferClosed.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		SelectionComponent->OnPendingCountChanged.
		                    RemoveDynamic(this, &UNSAugmentationWidget::HandlePendingCountChanged);
		SelectionComponent->OnRerollResult.RemoveDynamic(this, &UNSAugmentationWidget::HandleRerollResult);
	}

	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UNSAugmentationWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

UNSAugmentSelectionComponent* UNSAugmentationWidget::GetSelectionComponent()
{
	if (SelectionComponent.IsValid())
	{
		return SelectionComponent.Get();
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return nullptr;
	}

	SelectionComponent = PC->FindComponentByClass<UNSAugmentSelectionComponent>();

	return SelectionComponent.Get();
}

UNSAugmentInventoryComponent* UNSAugmentationWidget::GetInventoryComponent()
{
	if (InventoryComponent.IsValid())
	{
		return InventoryComponent.Get();
	}

	APlayerController* PC = GetOwningPlayer();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	if (!PS)
	{
		return nullptr;
	}

	InventoryComponent = PS->FindComponentByClass<UNSAugmentInventoryComponent>();

	return InventoryComponent.Get();
}

UNSCurrencyComponent* UNSAugmentationWidget::GetCurrencyComponent()
{
	if (CurrencyComponent.IsValid())
	{
		return CurrencyComponent.Get();
	}

	APlayerController* PC = GetOwningPlayer();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	if (!PS)
	{
		return nullptr;
	}

	CurrencyComponent = PS->FindComponentByClass<UNSCurrencyComponent>();

	return CurrencyComponent.Get();
}

void UNSAugmentationWidget::HandleOfferPresented(
	const TArray<FNSAugmentSelectionCard>& Cards,
	int64 RerollCost,
	bool bCanReroll,
	int32 OfferRevision,
	int32 MaxChoiceCount,
	int32 AvailableCardCount)
{
	const bool bWasRerollRequest =
		bRerollRequestPending;

	bRerollRequestPending = false;

	ClearSelectionAnimationTimer();

	CurrentOfferRevision = OfferRevision;
	CurrentRerollCost = RerollCost;

	ChoiceGuideSlotCount =
		NormalizeChoiceGuideSlotCount(
			MaxChoiceCount);

	CurrentAvailableCardCount =
		FMath::Max(
			AvailableCardCount,
			0);

	CurrentOfferCards = Cards;

	// 서버 오류 방어용. 정상 흐름에서는 초과하지 않아야 함.
	if (CurrentOfferCards.Num() >
		ChoiceGuideSlotCount)
	{
		CurrentOfferCards.SetNum(
			ChoiceGuideSlotCount);
	}

	const int32 SelectableChoiceCount =
		GetSelectableChoiceCount();

	const int32 ExpectedChoiceCount =
		FMath::Min(
			CurrentAvailableCardCount,
			ChoiceGuideSlotCount);

	const bool bHasExpectedChoiceCount =
		SelectableChoiceCount ==
		ExpectedChoiceCount;

	if (!bHasExpectedChoiceCount)
	{
		NS_OBJ_LOG(
			LogNS,
			Warning,
			"Received augment card count does not match the available candidate count. Expected={Expected}, Actual={Actual}, Available={Available}, MaxChoice={MaxChoice}",
			("Expected", ExpectedChoiceCount),
			("Actual", SelectableChoiceCount),
			("Available",
				CurrentAvailableCardCount),
			("MaxChoice",
				ChoiceGuideSlotCount));
	}

	bCanRerollCurrentOffer =
		bCanReroll &&
		bHasExpectedChoiceCount &&
		SelectableChoiceCount > 0;

	SetRerollStatusMessage(
		bWasRerollRequest
			? FText::FromString(
				TEXT("Reroll Complete"))
			: FText::GetEmpty());

	if (bWasRerollRequest &&
		SelectableChoiceCount > 0)
	{
		PlayAugmentRerollSuccessSound();
	}

	RefreshRerollControls();

	CurrentOfferViewData.Reset();
	CurrentOfferViewData.SetNum(
		SelectableChoiceCount);

	CurrentCardsPerSlot =
		CalculateCardsPerSlot(
			CurrentAvailableCardCount);

	CreateChoiceCard(
		SelectableChoiceCount);

	TArray<FSoftObjectPath> PathsToLoad;

	UNSAugmentDisplayBridgeSubsystem* DisplayBridge =
		GetGameInstance()
			? GetGameInstance()->GetSubsystem<
				UNSAugmentDisplayBridgeSubsystem>()
			: nullptr;

	UNSAugmentInventoryComponent* Inventory =
		GetInventoryComponent();

	if (DisplayBridge)
	{
		for (int32 Index = 0;
		     Index < SelectableChoiceCount;
		     ++Index)
		{
			if (!CurrentOfferCards.IsValidIndex(Index) ||
				!CurrentOfferViewData.IsValidIndex(Index))
			{
				continue;
			}

			const FNSAugmentSelectionCard& CardData =
				CurrentOfferCards[Index];

			const int32 CurrentStack =
				Inventory
					? Inventory->GetStackCount(CardData.DefId)
					: 0;

			FNSAugmentCardViewData& ViewData =
				CurrentOfferViewData[Index];

			if (!DisplayBridge->TryBuildCardViewData(
				CardData.DefId,
				CardData.Rarity,
				CurrentStack,
				ViewData))
			{
				continue;
			}

			if (!ViewData.Icon.IsNull())
			{
				PathsToLoad.AddUnique(
					ViewData.Icon.ToSoftObjectPath());
			}
		}
	}

	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (!PathsToLoad.IsEmpty())
	{
		IconLoadHandle =
			UAssetManager::GetStreamableManager().
			RequestAsyncLoad(
				PathsToLoad,
				FStreamableDelegate::CreateUObject(
					this,
					&UNSAugmentationWidget::OnIconsLoaded));
	}
	else
	{
		PopulateOfferCards();
		RefreshAugmentPanelState();
	}
}

void UNSAugmentationWidget::OnIconsLoaded()
{
	PopulateOfferCards();
	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::PopulateOfferCards()
{
	for (int32 Index = 0;
	     Index < AugmentCardWidgets.Num();
	     ++Index)
	{
		UNSAugmentCardWidget* Card = AugmentCardWidgets[Index];

		if (!Card || !CurrentOfferViewData.IsValidIndex(Index))
		{
			continue;
		}

		const FNSAugmentCardViewData& ViewData = CurrentOfferViewData[Index];

		if (!ViewData.DefId.IsValid())
		{
			continue;
		}

		Card->ApplyViewData(ViewData);
	}

	QueueChoiceCardPositionRefresh();
}

void UNSAugmentationWidget::HandleOfferClosed()
{
	// 오퍼가 닫힐 때 지연 선택 요청을 취소
	ClearSelectionAnimationTimer();

	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	HideCardSection();

	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->CloseAugmentationPanel();
	}
}

void UNSAugmentationWidget::HandlePendingCountChanged(int32 NewCount)
{
	if (PendingCountText)
	{
		if (NewCount <= 0)
		{
			PendingCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			PendingCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
			PendingCountText->SetText(FText::AsNumber(NewCount));
		}
	}

	if (RemainingAugmentCountText)
	{
		RemainingAugmentCountText->SetText(FText::AsNumber(NewCount));
	}

	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::HandleRerollResult(
	ENSAugmentRerollResult Result,
	int64 RequiredCost,
	int64 HaveCurrency,
	int32 RequestRevision,
	int32 ServerOfferRevision)
{
	bRerollRequestPending = false;

	if (RequestRevision != CurrentOfferRevision)
	{
		SetRerollStatusMessage(FText::GetEmpty());
		RefreshRerollControls();
		return;
	}

	CurrentOfferRevision = ServerOfferRevision;

	FText Message;
	switch (Result)
	{
	case ENSAugmentRerollResult::NotEnoughCurrency:
		Message = FText::FromString(FString::Printf(
			TEXT("임시 재화가 부족합니다. (보유 %lld / 필요 %lld)"),
			HaveCurrency,
			RequiredCost));
		break;

	case ENSAugmentRerollResult::NoDifferentOffer:
		Message = FText::FromString(
			TEXT("현재 조건에서 새로운 증강 카드를 만들 수 없습니다."));
		break;

	default:
		break;
	}

	if (!Message.IsEmpty())
	{
		PlayAugmentRerollFailSound();
	}

	SetRerollStatusMessage(Message);
	RefreshRerollControls();
}

void UNSAugmentationWidget::HandleInventoryChanged()
{
	if (bPanelOpen)
	{
		RefreshOwnedAugmentList();
	}

	if (UNSCharacterStatsBridgeSubsystem* StatsBridge =
		GetGameInstance()->GetSubsystem<UNSCharacterStatsBridgeSubsystem>())
	{
		StatsBridge->BroadcastCharacterStats(GetOwningPlayer());
	}
}

bool UNSAugmentationWidget::AreOwnedAugmentListReady() const
{
	return
		OwnedAugmentListRoot &&
		CommonAugmentSectionRoot &&
		RareAugmentSectionRoot &&
		EpicAugmentSectionRoot &&
		LegendaryAugmentSectionRoot &&
		CommonAugmentWrapBox &&
		RareAugmentWrapBox &&
		EpicAugmentWrapBox &&
		LegendaryAugmentWrapBox;
}

void UNSAugmentationWidget::ClearOwnedAugmentLists()
{
	if (CommonAugmentWrapBox)
	{
		CommonAugmentWrapBox->ClearChildren();
	}

	if (RareAugmentWrapBox)
	{
		RareAugmentWrapBox->ClearChildren();
	}

	if (EpicAugmentWrapBox)
	{
		EpicAugmentWrapBox->ClearChildren();
	}

	if (LegendaryAugmentWrapBox)
	{
		LegendaryAugmentWrapBox->ClearChildren();
	}
}

void UNSAugmentationWidget::RefreshOwnedAugmentSectionVisibility()
{
	if (!AreOwnedAugmentListReady())
	{
		return;
	}

	CommonAugmentSectionRoot->SetVisibility(
		CommonAugmentWrapBox->GetChildrenCount() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);

	RareAugmentSectionRoot->SetVisibility(
		RareAugmentWrapBox->GetChildrenCount() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);

	EpicAugmentSectionRoot->SetVisibility(
		EpicAugmentWrapBox->GetChildrenCount() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);

	LegendaryAugmentSectionRoot->SetVisibility(
		LegendaryAugmentWrapBox->GetChildrenCount() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
}

UWrapBox* UNSAugmentationWidget::GetOwnedAugmentWrapBox(
	ENSAugmentRarity Rarity) const
{
	switch (Rarity)
	{
	case ENSAugmentRarity::Common:
		return CommonAugmentWrapBox;

	case ENSAugmentRarity::Rare:
		return RareAugmentWrapBox;

	case ENSAugmentRarity::Epic:
		return EpicAugmentWrapBox;

	case ENSAugmentRarity::Legendary:
		return LegendaryAugmentWrapBox;

	default:
		return nullptr;
	}
}

void UNSAugmentationWidget::BeginCardSelection(int32 CardIndex)
{
	// 유효하지 않은 카드 선택이면 연출을 시작하지 않음
	if (!CurrentOfferCards.IsValidIndex(CardIndex) ||
		!AugmentCardWidgets.IsValidIndex(CardIndex))
	{
		return;
	}

	// 선택 애니메이션 중복 입력을 막음
	bSelectionAnimationPlaying = true;

	// 애니메이션 종료 후 선택 요청을 보낼 카드 번호를 저장
	PendingSelectedCardIndex = CardIndex;

	// 기존 하이라이트 인덱스를 선택 카드로 갱신
	HighlightedCardIndex = CardIndex;

	for (int32 Index = 0; Index < AugmentCardWidgets.Num(); ++Index)
	{
		UNSAugmentCardWidget* Card = AugmentCardWidgets[Index];
		if (!Card)
		{
			continue;
		}

		// 이전 선택 연출 상태를 먼저 초기화
		Card->ResetSelectionVisual();

		if (Index == CardIndex)
		{
			// 선택된 카드만 하이라이트와 선택 애니메이션을 적용
			Card->SetHighLighted(true);
			Card->PlaySelectAnimation();
		}
		else
		{
			// 선택되지 않은 카드는 하이라이트를 끄고 흐리게 만듦
			Card->SetHighLighted(false);
			Card->SetDeselectedVisual(DeselectedChoiceCardOpacity);
		}
	}

	// 선택 연출 중에는 가이드와 리롤 UI를 갱신해 숨김
	RefreshChoiceGuideVisibility();
	RefreshRerollControls();

	// 선택 애니메이션이 끝난 뒤 실제 서버 선택 요청
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SelectionAnimationTimerHandle);
		World->GetTimerManager().SetTimer(
			SelectionAnimationTimerHandle,
			this,
			&UNSAugmentationWidget::FinishPendingCardSelection,
			SelectionAnimationDelay,
			false);
		return;
	}

	// 월드 타이머를 사용할 수 없으면 즉시 선택 요청
	FinishPendingCardSelection();
}

void UNSAugmentationWidget::FinishPendingCardSelection()
{
	// 선택 요청에 사용할 카드 인덱스를 임시 저장
	const int32 CardIndex = PendingSelectedCardIndex;

	// 선택 대기 상태를 해제
	PendingSelectedCardIndex = INDEX_NONE;
	bSelectionAnimationPlaying = false;

	// 애니메이션 중 오퍼가 바뀌었다면 선택 연출만 초기화
	if (!CurrentOfferCards.IsValidIndex(CardIndex))
	{
		ResetChoiceCardSelectionVisuals();
		RefreshChoiceGuideVisibility();
		RefreshRerollControls();
		return;
	}

	// 선택 애니메이션이 끝난 뒤 실제 서버 선택 요청
	ConfirmAugmentSelection(CardIndex);
}

void UNSAugmentationWidget::ResetChoiceCardSelectionVisuals()
{
	// 선택 대기 상태를 초기화
	PendingSelectedCardIndex = INDEX_NONE;
	bSelectionAnimationPlaying = false;

	for (UNSAugmentCardWidget* Card : AugmentCardWidgets)
	{
		if (!Card)
		{
			continue;
		}

		// 각 카드의 선택 연출 상태를 기본값으로 되돌림
		Card->ResetSelectionVisual();
	}
}

void UNSAugmentationWidget::ClearSelectionAnimationTimer()
{
	// 지연 선택 타이머가 있으면 해제
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SelectionAnimationTimerHandle);
	}

	// 선택 대기 상태를 초기화
	PendingSelectedCardIndex = INDEX_NONE;
	bSelectionAnimationPlaying = false;
}

void UNSAugmentationWidget::PlayAugmentSound(FName SoundID) const
{
	if (SoundID.IsNone())
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
	{
		SoundSubsystem->PlaySound2D(SoundID);
	}
}

int32 UNSAugmentationWidget::NormalizeChoiceGuideSlotCount(int32 MaxChoiceCount) const
{
	return MaxChoiceCount >= 4 ? 4 : 3;
}

void UNSAugmentationWidget::
RefreshChoiceGuideSlotVisibility()
{
	const bool bUseThreeGuide =
		ChoiceGuideSlotCount == 3;

	const bool bUseFourGuide =
		ChoiceGuideSlotCount == 4;

	SetChoiceGuideSlotVisible(
		Choice3Slot1GuideRoot.Get(),
		bUseThreeGuide &&
		IsChoiceGuideSlotActive(0));

	SetChoiceGuideSlotVisible(
		Choice3Slot2GuideRoot.Get(),
		bUseThreeGuide &&
		IsChoiceGuideSlotActive(1));

	SetChoiceGuideSlotVisible(
		Choice3Slot3GuideRoot.Get(),
		bUseThreeGuide &&
		IsChoiceGuideSlotActive(2));

	SetChoiceGuideSlotVisible(
		Choice4Slot1GuideRoot.Get(),
		bUseFourGuide &&
		IsChoiceGuideSlotActive(0));

	SetChoiceGuideSlotVisible(
		Choice4Slot2GuideRoot.Get(),
		bUseFourGuide &&
		IsChoiceGuideSlotActive(1));

	SetChoiceGuideSlotVisible(
		Choice4Slot3GuideRoot.Get(),
		bUseFourGuide &&
		IsChoiceGuideSlotActive(2));

	SetChoiceGuideSlotVisible(
		Choice4Slot4GuideRoot.Get(),
		bUseFourGuide &&
		IsChoiceGuideSlotActive(3));
}

void UNSAugmentationWidget::SetChoiceGuideSlotVisible(UWidget* SlotWidget, bool bVisible) const
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->SetVisibility(
		bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden);
}
