// Copyright 2026 One Team. All rights reserved.


#include "NSTitleWidget.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"

void UNSTitleWidget::OnClickedHostButton()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance) return;
	
	UNSSessionSubsystem* SessionSubsystem =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>();
	if (!SessionSubsystem)
	{
		return;
	}
	//호스트 세션 생성 요청
	SessionSubsystem->StartGameToHub();
	UE_LOG(LogTemp, Warning, TEXT("호스트 방 생성 버튼 클릭"));
}

void UNSTitleWidget::OnClickedJoinButton()
{
	// 스팀 세션연동되었는지 확인용 (테스트 후에 지우고 밑의 주석 제거해야함)
	UNSSessionSubsystem* SessionSubsystem =
	   GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr;
	if (!SessionSubsystem) return;

	SessionSubsystem->FindAndJoinFirstSession(); 
	UE_LOG(LogTemp, Warning, TEXT("검색 기반 조인 시도"));
	
	//TODO(영웅): 호스트가 생성한 방에 들어가는 로직
	//참가할 IP를 입력할수 있게 패널 표시
	/*
	if (JoinPanel)
	{
		JoinPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (IPTextBox)
	{
		IPTextBox->SetFocus();
	}
	UE_LOG(LogTemp, Warning, TEXT("참가 버튼 클릭"));
	*/
}

void UNSTitleWidget::OnClickedOptionButton()
{
	//TODO(영웅): 옵션 UI 연결
	
	UE_LOG(LogTemp, Warning, TEXT("옵션 버튼 클릭"));
	
	UGameInstance* GameInstance = GetGameInstance();
	UNSUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (UIManager)
	{
		UIManager->OpenOptionPanel(GetOwningPlayer());
	}
}

void UNSTitleWidget::OnClickedQuitButton()
{
	//게임 종료처리
	
	UKismetSystemLibrary::QuitGame(
		this,
		GetOwningPlayer(),
		EQuitPreference::Quit,
		false);
}

void UNSTitleWidget::OnClickedConfirmJoinButton()
{
	if (!IPTextBox)
	{
		return;
	}
	const FString InputAddress = IPTextBox->GetText().ToString();
	
	if (InputAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("IP 입력 창"));
		return;
	}
	UNSSessionSubsystem* SessionSubsystem =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>();
	if (!SessionSubsystem)
	{
		return;
	}
	//입력한 IP 주소로 참가 요청
	SessionSubsystem->JoinSessionByAddress(InputAddress);
	UE_LOG(LogTemp,Warning,TEXT("참가 : %s"), *InputAddress);
}

void UNSTitleWidget::OnClickedCancelJoinButton()
{
	//입력 패널을 닫고 타이틀 상태로 돌아오기
	if (JoinPanel)
	{
		JoinPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNSTitleWidget::OnChangedIPText(const FText& ChangedText)
{
	if (!IPTextBox)
	{
		return;
	}
	FString InputString = ChangedText.ToString();
	
	FString FilteredString;
	//숫자와 . 만 허용
	for (TCHAR Character : InputString)
	{
		if (FChar::IsDigit(Character) || Character == TEXT('.'))
		{
			FilteredString.AppendChar(Character);
		}
	}
	//허용되지 않은 문자가 제거된 문자열로 갱신
	if (InputString != FilteredString)
	{
		IPTextBox->SetText(FText::FromString(FilteredString));
	}
}

void UNSTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		//타이틀 화면에서 마우스커서가 보인다
		PlayerController->bShowMouseCursor = true;
		
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	
	//방 생성 버튼 클릭시 이벤트
	if (HostButton)
	{

		HostButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedHostButton);
	}
	
	
	//방 참가 버튼 클릭 이벤트
	if (JoinButton)
	{
		JoinButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedJoinButton);
	}
	//설정 버튼 클릭 이벤트
	if (OptionButton)
	{
		OptionButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedOptionButton);
	}
	//종료 버튼 클릭 이벤트
	if (QuitButton)
	{
		QuitButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedQuitButton);
	}
	//참가 패널
	if (JoinPanel)
	{
		JoinPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	//IP입력
	if (IPTextBox)
	{
		IPTextBox->OnTextChanged.AddDynamic(
			this,
			&UNSTitleWidget::OnChangedIPText);
	}
	//참가 확인 버튼 클릭 이벤트
	if (ConfirmJoinButton)
	{
		ConfirmJoinButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedConfirmJoinButton);
	}
	//참가 취소 버튼 클릭 이벤트
	if (CancelJoinButton)
	{
		CancelJoinButton->OnClicked().AddUObject(
			this,
			&UNSTitleWidget::OnClickedCancelJoinButton);
	}
}
