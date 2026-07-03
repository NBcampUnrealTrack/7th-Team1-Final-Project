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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"


void UNSAugmentationWidget::OpenPanel()
{
	// 순수 UI 표시만 담당 (보유 아이콘 갱신)
	bPanelOpen = true;
	SetVisibility(ESlateVisibility::Visible);
	RefreshOwnedAugmentList();
}

void UNSAugmentationWidget::ClosePanel()
{
	bPanelOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	
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
			
			FVector2D CardPosition;
			switch (Index)
			{
			case 0:
				//1번 선택지: 왼쪽아래
				CardPosition = FVector2D(-350.f,200.f);
				break;
			case 1:
				//2번 선택지: 중앙 위
				CardPosition = FVector2D(0.0f,100.f);
				break;
			case 2:
				//3번 선택지 오른쪽 아래
				CardPosition = FVector2D(350.f,200.f);
				break;
			default:
				CardPosition = FVector2D(0.0f,0.0f);
				break;
			}
			CardSlot->SetPosition(CardPosition);
		}
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
	SelComp->Server_Choose(CardIndex);
}

void UNSAugmentationWidget::RequestRerollAugment()
{
	// 재화/리롤 횟수 검증은 서버(Server_RerollCard)에서 처리 예정 (현재 재화 시스템 미연동)
	UNSAugmentSelectionComponent* SelComp = GetSelectionComponent();
	if (!SelComp)
	{
		return;
	}
	//서버에 전체 리롤 요청 → Client_PresentOffer → HandleOfferPresented로 카드 갱신
	SelComp->Server_RerollCard();
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

void UNSAugmentationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//기본상태에서는 패널 숨김 + 카드 영역 숨김
	bPanelOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	HideCardSection();

	//오너 PC의 선택 컴포넌트 델리게이트 구독
	APlayerController* PC = GetOwningPlayer();

	if (UNSAugmentSelectionComponent* SelComp = GetSelectionComponent())
	{
		SelComp->OnOfferPresented.AddDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelComp->OnOfferClosed.AddDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		SelComp->OnPendingCountChanged.AddDynamic(this, &UNSAugmentationWidget::HandlePendingCountChanged);
		//현재 대기 카운트로 뱃지 초기화
		HandlePendingCountChanged(SelComp->GetPendingCount());
	}

	//보유 증강 변경 구독
	if (UNSAugmentInventoryComponent* Inv = GetInventoryComponent())
	{
		Inv->OnInventoryChanged.AddDynamic(this, &UNSAugmentationWidget::HandleInventoryChanged);
	}
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

void UNSAugmentationWidget::HandleOfferPresented(const TArray<FNSAugmentSelectionCard>& Cards, int32 RerollCost)
{
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
		ShowCardSection();
	}
}

void UNSAugmentationWidget::OnIconsLoaded()
{
	PopulateOfferCards();
	ShowCardSection();
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
		UIManager->ClosePartPanel();
	}
}

void UNSAugmentationWidget::HandlePendingCountChanged(int32 NewCount)
{
	if (!PendingCountText)
	{
		return;
	}
	//대기 0개면 뱃지 숨김
	if (NewCount <= 0)
	{
		PendingCountText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PendingCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	PendingCountText->SetText(FText::AsNumber(NewCount));
}

void UNSAugmentationWidget::HandleInventoryChanged()
{
	//패널이 열려 있을 때만 보유 아이콘 갱신 (닫혀 있으면 다음 OpenPanel에서 갱신)
	if (bPanelOpen)
	{
		RefreshOwnedAugmentList();
	}
}
