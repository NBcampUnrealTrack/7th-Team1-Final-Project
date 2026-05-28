// Copyright 2026 One Team. All rights reserved.

#include "Components/CanvasPanelSlot.h"
#include "NeoSanctum/UI/HUD/NSAugmentCardWidget.h"
#include "NSAugmentationWidget.h"
#include "Components/CanvasPanel.h"

void UNSAugmentationWidget::ShowAugmentation()
{
	//증강 선택 UI 표시
	SetVisibility(ESlateVisibility::Visible);
	
	SetKeyboardFocus();
}

void UNSAugmentationWidget::HideAugmentation()
{
	//증강 선택 UI 숨김
	SetVisibility(ESlateVisibility::Collapsed);
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
		//테스트용 임시 데이터
		NewCard->SetAugmentName(
			FString::Printf(
				TEXT("증강 선택지 %d"),Index +1));
		NewCard->SetAugmentDescription(
			TEXT("증강 설명 테스트"));
		//HorizontalBox에 증강추가
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
			}
			CardSlot->SetPosition(CardPosition);
		}
	}
}

void UNSAugmentationWidget::SelectCardByIndex(int32 CardIndex)
{
	if (!AugmentCardWidgetClass)
	{
		return;
	}
	HighLightCard(CardIndex);
	UE_LOG(LogTemp,Warning,TEXT("증강 선택 확정 : %d"),CardIndex+1);
	//TODO(영웅): 선택한 증강 선택 연결 로직
	HideAugmentation();
}

void UNSAugmentationWidget::ConfirmAugmentSelection(int32 CardIndex)
{
	// TODO(영웅): 실제 증강 데이터 선택 및 적용 로직 연결
	UE_LOG(LogTemp, Warning, TEXT("Augment Selected: %d"), CardIndex + 1);

	HideAugmentation();
}

void UNSAugmentationWidget::RequestRerollAugment()
{
	// TODO(영웅): 런 인 재화 또는 리롤 가능 횟수 확인 후 선택지 재생성
	UE_LOG(LogTemp, Warning, TEXT("Augment Reroll Requested"));

	CreateChoiceCard(ChoiceCount);
}

void UNSAugmentationWidget::RefreshOwnedAugmentList()
{
	// TODO(영웅): 현재 보유 중인 증강 목록 UI 갱신
	UE_LOG(LogTemp, Warning, TEXT("Refresh Owned Augment List"));
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
	
	//기본 상태에서는 숨김
	SetVisibility(ESlateVisibility::Collapsed);
	
	CreateChoiceCard(3);
	
	SetIsFocusable(true);
}

FReply UNSAugmentationWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();
	if (PressedKey == EKeys::One)
	{
		SelectCardByIndex(0);
		return FReply::Handled();
	}if (PressedKey == EKeys::Two)
	{
		SelectCardByIndex(1);
		return FReply::Handled();
	}if (PressedKey == EKeys::Three)
	{
		SelectCardByIndex(2);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
