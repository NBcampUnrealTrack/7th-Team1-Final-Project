// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentationWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "NeoSanctum/UI/HUD/NSAugmentCardWidget.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/HUD/NSAugmentDisplayBridgeSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"


void UNSAugmentationWidget::OpenPanel()
{
	// 순수 UI 표시만 담당 (보유 아이콘 갱신)
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
}

void UNSAugmentationWidget::HideCardSection()
{
	if (ChoiceRootCanvas)
	{
		ChoiceRootCanvas->ClearChildren();
	}
	AugmentCardWidgets.Empty();
	CurrentOfferCards.Reset();

	if (CardSectionRoot)
	{
		CardSectionRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSAugmentationWidget::CreateChoiceCard(int32 NewChoiceCount)
{
	//카드가 들어갈 박스가 없으면 생성 불가
	if (!ChoiceRootCanvas)
	{
		return;
	}

	// 스코프 초과 방어: 3/4장만 지원. 서버/데이터 문제를 조용히 숨기지 않고 Warning으로 드러냄.
	if (NewChoiceCount > 4)
	{
		NS_OBJ_LOG(LogNS, Warning,
			"[AugmentationWidget] 지원하지 않는 카드 수라 생성하지 않습니다. Count={Count}",
			("Count", NewChoiceCount)
		);
		return;
	}

	//기존 가드 제거
	ChoiceRootCanvas->ClearChildren();
	AugmentCardWidgets.Empty();
	
	ChoiceCount = NewChoiceCount;
	//생성할 카드위젯이 없으면 선택지가 생기지 않는다
	if (!AugmentCardWidgetClass)
	{
		return;
	}
	
	for (int32 Index = 0; Index < ChoiceCount; ++Index)
	{
		//증강 카드 위젯 생성
		UNSAugmentCardWidget* NewCard =
			CreateWidget<UNSAugmentCardWidget>(
				this,
				AugmentCardWidgetClass);
		if (!NewCard)
		{
			continue;
		}
		AugmentCardWidgets.Add(NewCard);
		
			NewCard->SetShortcutNumber(Index + 1);
			NewCard->SetAugmentName(TEXT(""));
    		NewCard->SetAugmentDescription(TEXT(""));
    		NewCard->SetAugmentIcon(nullptr);
    
    		UCanvasPanelSlot* CardSlot =
    			ChoiceRootCanvas->AddChildToCanvas(NewCard);

		if (CardSlot)
		{
			//증강 카드의 중심점을 기준으로 위치를 잡는다
			CardSlot->SetAutoSize(true);
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));

			CardSlot->SetPosition(GetChoiceCardPosition(Index));
		}
	}
}

FVector2D UNSAugmentationWidget::GetChoiceCardPosition(int32 Index) const
{
	if (ChoiceCount == 4)
	{
		switch (Index)
		{
		case 0: return LeftCardPosition;
		case 1: return FourCardCenterLeftPosition;
		case 2: return FourCardCenterRightPosition;
		case 3: return RightCardPosition;
		default: return FVector2D::ZeroVector;
		}
	}

	switch (Index)
	{
	case 0: return LeftCardPosition;
	case 1: return ThreeCardCenterPosition;
	case 2: return RightCardPosition;
	default: return FVector2D::ZeroVector;
	}
}

void UNSAugmentationWidget::SelectCardByIndex(int32 CardIndex)
{
	//잘못된 번호가 입력되면 선택 x
	if (!AugmentCardWidgets.IsValidIndex(CardIndex))
	{
		return;
	}

	ConfirmAugmentSelection(CardIndex);
}

void UNSAugmentationWidget::ConfirmAugmentSelection(int32 CardIndex)
{
	// 리롤 응답을 기다리는 중이면 카드 선택도 막음.
	if (bRerollRequestPending)
	{
		return;
	}

	//현재 오퍼 범위 밖이면 무시
	if (!CurrentOfferCards.IsValidIndex(CardIndex))
	{
		return;
	}

	UNSAugmentSelectionComponent* SelComp = GetSelectionComponent();
	if (!SelComp)
	{
		return;
	}

	//서버 권한에서 증강 적용. UI 숨김은 서버의 Client_CloseOffer -> OnOfferClosed가 처리
	SelComp->Server_Choose(CardIndex, CurrentOfferRevision);
}

void UNSAugmentationWidget::RequestRerollAugment()
{
	// 이미 요청을 보내놨거나, 지금 오퍼에서 리롤이 안되는 상태면 무시 (버튼 숨김만으로는 T키를 못 막으니 여기서도 막음)
	if (bRerollRequestPending || !bCanRerollCurrentOffer)
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

	//서버에 전체 리롤 요청 → Client_PresentOffer → HandleOfferPresented로 카드 갱신
	SelComp->Server_RerollCard(CurrentOfferRevision);
}

bool UNSAugmentationWidget::CanAffordReroll()
{
	const UNSCurrencyComponent* CurrencyComp = GetCurrencyComponent();
	return CurrencyComp && CurrencyComp->GetTemp() >= CurrentRerollCost;
}

bool UNSAugmentationWidget::IsRerollAvailable()
{
	return !bRerollRequestPending && CanAffordReroll();
}

void UNSAugmentationWidget::RefreshRerollControls()
{
	const ESlateVisibility RerollVisibility =
		bCanRerollCurrentOffer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (RerollButton)
	{
		RerollButton->SetVisibility(RerollVisibility);
	}

	if (RerollCostText)
	{
		RerollCostText->SetVisibility(RerollVisibility);
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
	RerollStatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UNSAugmentationWidget::RefreshOwnedAugmentList()
{
	if (!OwnedAugmentWrapBox)
	{
		return;
	}
	OwnedAugmentWrapBox->ClearChildren();

	UNSAugmentInventoryComponent* Inv = GetInventoryComponent();
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Inv || !Data || !Data->IsRunReady())
	{
		return;
	}

	// 보유 증강 아이콘 소프트포인터 수집
	TArray<FSoftObjectPath> PathsToLoad;
	for (const FNSAugmentInstance& Inst : Inv->GetOwned())
	{
		const UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(Inst.DefId);
		if (Def && !Def->Icon.IsNull())
		{
			PathsToLoad.Add(Def->Icon.ToSoftObjectPath());
		}
	}
	if (PathsToLoad.Num() == 0)
	{
		return;
	}

	// 이전 로드 취소 후 비동기 로드
	if (OwnedIconLoadHandle.IsValid())
	{
		OwnedIconLoadHandle->CancelHandle();
		OwnedIconLoadHandle.Reset();
	}
	OwnedIconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PathsToLoad,
		FStreamableDelegate::CreateUObject(this, &UNSAugmentationWidget::OnOwnedIconsLoaded)
	);
}

void UNSAugmentationWidget::OnOwnedIconsLoaded()
{
	if (!OwnedAugmentWrapBox || !WidgetTree)
	{
		return;
	}
	OwnedAugmentWrapBox->ClearChildren();

	UNSAugmentInventoryComponent* Inv = GetInventoryComponent();
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Inv || !Data)
	{
		return;
	}
	
	for (const FNSAugmentInstance& Inst : Inv->GetOwned())
	{
		const UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(Inst.DefId);
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
		// SizeBox로 감싸서 텍스처 원본 해상도와 무관하게 일정한 크기로 표시
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


		// SetBrushFromTexture 후 원하는 표시 크기로 재지정
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
			UBorder* CountBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

			if (CountBadge && CountText)
			{
				CountBadge->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.85f));
				CountBadge->SetPadding(FMargin(4.f, 1.f));

				FSlateFontInfo FontInfo = CountText->GetFont();
				FontInfo.Size = 14;
				CountText->SetFont(FontInfo);

				CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				CountText->SetText(FText::Format(
					NSLOCTEXT("AugmentationWidget", "OwnedAugmentCount", "{0}"),
					FText::AsNumber(StackCount)
				));

				CountBadge->AddChild(CountText);

				UOverlaySlot* BadgeSlot = Overlay->AddChildToOverlay(CountBadge);
				if (BadgeSlot)
				{
					BadgeSlot->SetHorizontalAlignment(HAlign_Right);
					BadgeSlot->SetVerticalAlignment(VAlign_Bottom);
				}
			}
		}

		SizeBox->AddChild(Overlay);
		OwnedAugmentWrapBox->AddChildToWrapBox(SizeBox);
	}
}

void UNSAugmentationWidget::HighLightCard(int32 CardIndex)
{

	//잘못된 인덱스가 들어온경우 처리 x
	if (!AugmentCardWidgets.IsValidIndex(CardIndex))
	{
		return;
	}
	HighlightedCardIndex= CardIndex;
	for (int32 Index = 0; Index < AugmentCardWidgets.Num(); ++Index)
	{
		if (!AugmentCardWidgets[Index])
		{
			continue;
		}
		//선택한 카드만 강조
		AugmentCardWidgets[Index]->SetHighLighted(Index == HighlightedCardIndex);
	}
}

void UNSAugmentationWidget::RefreshAugmentPanelState()
{
	const UNSAugmentSelectionComponent* SelComp = SelectionComponent.Get();
	const int32 PendingCount = SelComp ? SelComp->GetPendingCount() : 0;

	const bool bHasPendingAugment = PendingCount > 0;
	const bool bHasOfferCards = CurrentOfferCards.Num() > 0;
	// 위젯 자체는 Tab으로 열렸거나(bPanelOpen), C로 보유 목록이 요청됐거나(bOwnedListRequested),
	// 새로 고를 증강이 대기 중이면(알림 뱃지) 보여준다. 셋 중 하나라도 아니면
	// 대기 오퍼가 없을 때 Tab/C를 눌러도 보유 목록이 안 보이는 문제가 생긴다.
	const bool bShouldShowAugmentNotice = bPanelOpen || bOwnedListRequested || bHasPendingAugment;
	// 카드 선택 화면은 실제 카드 데이터가 있고, Tab으로 패널을 연 상태에서만 보여준다.
	const bool bShouldShowCardSection = bPanelOpen && bHasOfferCards;
	if (!bShouldShowAugmentNotice)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		HideCardSection();

		if (CenterControlRoot)
		{
			CenterControlRoot->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		if (CardDimBackground)
		{
			CardDimBackground->SetVisibility(ESlateVisibility::Collapsed);
		}

		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	if (CenterControlRoot)
	{
		CenterControlRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (CardSectionRoot)
	{
		CardSectionRoot->SetVisibility(
			bShouldShowCardSection
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (CardDimBackground)
	{
		CardDimBackground->SetVisibility(
			bShouldShowCardSection
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (PendingCountText)
	{
		PendingCountText->SetText(FText::AsNumber(PendingCount));
	}

	if (RemainingAugmentCountText)
	{
		RemainingAugmentCountText->SetText(FText::AsNumber(PendingCount));
	}
}

void UNSAugmentationWidget::SetOwnedAugmentListVisible(bool bVisible)
{
	bOwnedListRequested = bVisible;

	if (OwnedAugmentWrapBox)
	{
		OwnedAugmentWrapBox->SetVisibility(
			bVisible
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	RefreshAugmentPanelState();
}

void UNSAugmentationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//기본상태에서는 패널 숨김 + 카드 영역 숨김
	bPanelOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	HideCardSection();
	SetOwnedAugmentListVisible(false);

	//오너 PC의 선택 컴포넌트 델리게이트 구독
	if (UNSAugmentSelectionComponent* SelComp = GetSelectionComponent())
	{
		SelComp->OnOfferPresented.AddDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelComp->OnOfferClosed.AddDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		SelComp->OnPendingCountChanged.AddDynamic(this, &UNSAugmentationWidget::HandlePendingCountChanged);
		SelComp->OnRerollResult.AddDynamic(this, &UNSAugmentationWidget::HandleRerollResult);
		//현재 대기 카운트로 뱃지 초기화
		HandlePendingCountChanged(SelComp->GetPendingCount());
	}

	//보유 증강 변경 구독
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
}

void UNSAugmentationWidget::NativeDestruct()
{
	//진행 중인 비동기 로드 취소
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
	//구독 해제
	if (SelectionComponent.IsValid())
	{
		SelectionComponent->OnOfferPresented.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelectionComponent->OnOfferClosed.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		SelectionComponent->OnPendingCountChanged.RemoveDynamic(this, &UNSAugmentationWidget::HandlePendingCountChanged);
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
	int32 OfferRevision)
{
	// 지금 잠겨있던 것이 리롤 요청이었는지 먼저 기억해두고 바로 품 (리롤 완료 문구 표시용)
	const bool bWasRerollRequest = bRerollRequestPending;
	bRerollRequestPending = false;

	CurrentOfferRevision = OfferRevision;
	CurrentRerollCost = RerollCost;
	bCanRerollCurrentOffer = bCanReroll;

	SetRerollStatusMessage(bWasRerollRequest ? FText::FromString(TEXT("리롤 완료")) : FText::GetEmpty());
	RefreshRerollControls();

	CurrentOfferCards = Cards;
	CurrentOfferViewData.Reset();
	CurrentOfferViewData.SetNum(Cards.Num());

	CreateChoiceCard(Cards.Num());

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
		for (int32 Index = 0; Index < Cards.Num(); ++Index)
		{
			const FNSAugmentSelectionCard& CardData = Cards[Index];

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
			UAssetManager::GetStreamableManager().RequestAsyncLoad(
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
		UNSAugmentCardWidget* Card =
			AugmentCardWidgets[Index];

		if (!Card ||
			!CurrentOfferViewData.IsValidIndex(Index))
		{
			continue;
		}

		const FNSAugmentCardViewData& ViewData =
			CurrentOfferViewData[Index];

		if (!ViewData.DefId.IsValid())
		{
			continue;
		}

		Card->ApplyViewData(ViewData);
	}
}

void UNSAugmentationWidget::HandleOfferClosed()
{
	//오퍼 종료 시 아이콘 로드 핸들 해제 (자산 반환)
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}
	//카드 영역만 닫음. 패널 전체(보유 아이콘)는 유지 → 대기 0개면 보유 목록만 표시됨
	HideCardSection();
	if (UNSUIManagerSubsystem* UIManager =
		UNSUIManagerSubsystem::Get(this))
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

	// 응답이 내가 보낸 요청 기준이 아니면(그 사이 오퍼가 이미 바뀌었으면) 문구는 안띄우고 잠금만 품
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
			TEXT("임시 재화가 부족합니다. (보유 %lld / 필요 %lld)"), HaveCurrency, RequiredCost)
		);
		break;

	case ENSAugmentRerollResult::NoDifferentOffer:
		Message = FText::FromString(TEXT("현재 조건에서 새로운 증강 카드를 만들 수 없습니다."));
		break;

	default:
		// InvalidRequest/NoActiveOffer/StaleRevision은 사용자가 딱히 알 필요 없는 상황이라 조용히 잠금만 풀어줌.
		break;
	}

	SetRerollStatusMessage(Message);
	RefreshRerollControls();
}

void UNSAugmentationWidget::HandleInventoryChanged()
{
	//패널이 열려 있을 때만 보유 아이콘 갱신 (닫혀 있으면 다음 OpenPanel에서 갱신)
	if (bPanelOpen)
	{
		RefreshOwnedAugmentList();
	}
}
