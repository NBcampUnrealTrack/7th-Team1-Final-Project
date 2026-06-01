// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentationWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "NeoSanctum/UI/HUD/NSAugmentCardWidget.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

void UNSAugmentationWidget::ShowAugmentation()
{	
	//증강 UI표시
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	//OwningPlayer가 없으면 PlayerController사용
	APlayerController* PC = GetOwningPlayer();

	if (!PC && GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController();
	}

	if (!PC)
	{
		return;
	}
	//증강 선택중에는 입력모드 UI로 변경
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
	
	// 현재 증강 위젯에 키보드 포커스를 준다
	SetUserFocus(PC);
	SetKeyboardFocus();

}

void UNSAugmentationWidget::HideAugmentation()
{
	//증강 선택 UI 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	//증강 선택이 끝나면 입력 모드를 다시 게임 전용으로 복구
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
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

	UE_LOG(LogTemp, Warning, TEXT("[증강] 증강 선택 확정 : %d"), CardIndex + 1);

	HideAugmentation();
}

void UNSAugmentationWidget::ConfirmAugmentSelection(int32 CardIndex)
{
	// TODO(영웅): 실제 증강 데이터 선택 및 적용 로직 연결
	// 선택이 끝나면 증강 UI를 닫음
	HideAugmentation();
}

void UNSAugmentationWidget::RequestRerollAugment()
{
	// TODO(영웅): 런 인 재화 또는 리롤 가능 횟수 확인 후 선택지 재생성

	CreateChoiceCard(ChoiceCount);
}

void UNSAugmentationWidget::RefreshOwnedAugmentList()
{
	// TODO(영웅): 현재 보유 중인 증강 목록 UI 갱신
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
	//키보드입력을 받을수있게 한다
	SetIsFocusable(true);
	//기본상태에서는 숨김
	SetVisibility(ESlateVisibility::Collapsed);
	//증강 선택지 3개
	CreateChoiceCard(3);

}

FReply UNSAugmentationWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();
	
	//숫자키 1,2,3으로 각 증강 선택지를 바로 선택
	if (PressedKey == EKeys::One)
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강] 1번 키 입력"));
		SelectCardByIndex(0);
		return FReply::Handled();
	}if (PressedKey == EKeys::Two)
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강] 2번 키 입력"));
		SelectCardByIndex(1);
		return FReply::Handled();
	}if (PressedKey == EKeys::Three)
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강] 3번 키 입력"));
		SelectCardByIndex(2);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
