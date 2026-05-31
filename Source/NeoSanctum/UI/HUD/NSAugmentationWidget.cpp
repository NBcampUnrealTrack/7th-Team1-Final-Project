// Copyright 2026 One Team. All rights reserved.

#include "NSAugmentationWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "NeoSanctum/UI/HUD/NSAugmentCardWidget.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/Augment/NSAugmentDefinition.h"
#include "Engine/AssetManager.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"

void UNSAugmentationWidget::ShowAugmentation()
{
	//증강 UI표시
	SetVisibility(ESlateVisibility::Visible);

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
	
	// 입력 차단은 IMC 스위칭으로 달성
	if (UNSInputBinderComponent* Binder = GetOwningInputBinder(PC))
	{
		Binder->EnterAugmentInputMode();
	}
}

void UNSAugmentationWidget::HideAugmentation()
{
	//증강 선택 UI 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	if (UNSInputBinderComponent* Binder = GetOwningInputBinder(GetOwningPlayer()))
	{
		Binder->ExitAugmentInputMode();
	}
}

UNSInputBinderComponent* UNSAugmentationWidget::GetOwningInputBinder(APlayerController* PC) const
{
	ANSPlayerCharacterBase* Character = PC ? Cast<ANSPlayerCharacterBase>(PC->GetPawn()) : nullptr;
	return Character ? Character->GetInputBinderComponent() : nullptr;
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

	ConfirmAugmentSelection(CardIndex);
}

void UNSAugmentationWidget::ConfirmAugmentSelection(int32 CardIndex)
{
	//현재 오퍼 범위 밖이면 무시
	if (!CurrentOfferIds.IsValidIndex(CardIndex))
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
	//기본상태에서는 숨김
	SetVisibility(ESlateVisibility::Collapsed);
	//증강 선택지 3개
	CreateChoiceCard(3);

	//오너 PC의 선택 컴포넌트 델리게이트 구독
	APlayerController* PC = GetOwningPlayer();
	UE_LOG(LogTemp, Log, TEXT("[증강][Widget] NativeConstruct - PC=%s"), PC ? *PC->GetName() : TEXT("null"));

	if (UNSAugmentSelectionComponent* SelComp = GetSelectionComponent())
	{
		SelComp->OnOfferPresented.AddDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelComp->OnOfferClosed.AddDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
		UE_LOG(LogTemp, Log, TEXT("[증강][Widget] 델리게이트 바인딩 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강][Widget] SelectionComponent 없음 - 바인딩 실패"));
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
	//구독 해제
	if (SelectionComponent.IsValid())
	{
		SelectionComponent->OnOfferPresented.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferPresented);
		SelectionComponent->OnOfferClosed.RemoveDynamic(this, &UNSAugmentationWidget::HandleOfferClosed);
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

void UNSAugmentationWidget::HandleOfferPresented(const TArray<FPrimaryAssetId>& OfferIds, int32 RerollCost)
{
	UE_LOG(LogTemp, Log, TEXT("[증강][Widget] HandleOfferPresented - 카드 %d장"), OfferIds.Num());
	CurrentOfferIds = OfferIds;
	CreateChoiceCard(OfferIds.Num());

	// 오퍼 3개의 아이콘 소프트포인터만 수집 (GE/GA는 서버 ApplyAugment에서 로드)
	TArray<FSoftObjectPath> PathsToLoad;
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (Data && Data->IsRunReady())
	{
		for (const FPrimaryAssetId& Id : OfferIds)
		{
			const UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(Id);
			if (!Def)
			{
				continue;
			}
			if (!Def->Icon.IsNull())
			{
				PathsToLoad.Add(Def->Icon.ToSoftObjectPath());
			}
		}
	}

	// 이전 로드가 진행 중이면 취소 (리롤 시)
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (PathsToLoad.Num() > 0)
	{
		// 로드 완료 후 카드 채우고 UI 표시
		IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			PathsToLoad,
			FStreamableDelegate::CreateUObject(this, &UNSAugmentationWidget::OnIconsLoaded)
		);
	}
	else
	{
		// 로드할 소프트포인터가 없으면 (아이콘 미설정 등) 바로 표시
		PopulateOfferCards();
		if (GetVisibility() != ESlateVisibility::Visible)
		{
			ShowAugmentation();
		}
	}
}

void UNSAugmentationWidget::OnIconsLoaded()
{
	UE_LOG(LogTemp, Log, TEXT("[증강][Widget] OnIconsLoaded - 카드 채우기 시작"));
	PopulateOfferCards();
	// 첫 오퍼 시에만 UI를 열고, 리롤 시에는 이미 열려있으므로 생략
	if (GetVisibility() != ESlateVisibility::Visible)
	{
		ShowAugmentation();
	}
}

void UNSAugmentationWidget::PopulateOfferCards()
{
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("[증강] PopulateOfferCards: DataSubsystem 없음"));
		return;
	}
	if (!Data->IsRunReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("[증강] PopulateOfferCards: IsRunReady=false (Phase=%d)"), (int32)Data->GetCurrentPhase());
		return;
	}

	for (int32 Index = 0; Index < AugmentCardWidgets.Num(); ++Index)
	{
		UNSAugmentCardWidget* Card = AugmentCardWidgets[Index];
		if (!Card || !CurrentOfferIds.IsValidIndex(Index))
		{
			continue;
		}

		const UNSAugmentDefinition* Def = Data->GetData<UNSAugmentDefinition>(CurrentOfferIds[Index]);
		if (!Def)
		{
			UE_LOG(LogTemp, Warning, TEXT("[증강] PopulateOfferCards: [%d] Def 없음 Id=%s"), Index, *CurrentOfferIds[Index].ToString());
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("[증강] PopulateOfferCards: [%d] %s"), Index, *Def->DisplayName.ToString());
		Card->SetAugmentName(Def->DisplayName.ToString());
		Card->SetAugmentDescription(Def->Description.ToString());
		// 비동기 로드 완료 후 호출되므로 .Get()으로 바로 사용 가능
		Card->SetAugmentIcon(Def->Icon.Get());
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
	//서버가 오퍼를 닫으면 UI 숨김
	HideAugmentation();
}
