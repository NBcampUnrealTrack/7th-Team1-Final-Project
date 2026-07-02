// Copyright 2026 One Team. All rights reserved.


#include "NSPartyConsoleWidget.h"
#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "HAL/PlatformApplicationMisc.h"


void UNSPartyConsoleWidget::OpenForInteractor(APlayerController* Interactor)
{
	OwningPC = Interactor;

	AddToViewport();

	UNSSessionSubsystem* Session =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr;
	
	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked().AddUObject(
			this,
			&UNSPartyConsoleWidget::OnClickedCreateSession);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this,
			&UNSPartyConsoleWidget::OnClickedClose);
	}
	if (CopyCodeButton)
	{
		CopyCodeButton->OnClicked().AddUObject(
			this,
			&UNSPartyConsoleWidget::OnClickedCopyCode);
	}
	
	// 초대 코드 발급 구독
	if (Session)
	{
		Session->OnInviteCodeReady.RemoveDynamic(
			this,
			&UNSPartyConsoleWidget::HandleInviteCodeReady);
		Session->OnInviteCodeReady.AddDynamic(
			this,
			&UNSPartyConsoleWidget::HandleInviteCodeReady);

		// 이미 세션이 있으면 기존 코드 즉시 표시
		const FString ExistingCode = Session->GetCurrentInviteCode();
		if (!ExistingCode.IsEmpty() && InviteCodeText)
		{
			InviteCodeText->SetText(FText::FromString(ExistingCode));
		}
	}

	// 입력 모드: 허브에서 계속 돌아다닐 수 있게 게임+UI
	if (Interactor)
	{
		Interactor->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Interactor->SetInputMode(InputMode);
	}
}

void UNSPartyConsoleWidget::CloseWidget()
{
	if (UNSSessionSubsystem* Session =
		GetGameInstance() ?
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr)
	{
		Session->OnInviteCodeReady.RemoveDynamic(
			this,
			&UNSPartyConsoleWidget::HandleInviteCodeReady);
	}
	
	if (APlayerController* PC = OwningPC.Get())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	
	RemoveFromParent();
}

void UNSPartyConsoleWidget::OnClickedCreateSession()
{
	if (UNSSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>())
	{
		Session->CreateSession();
	}
}

void UNSPartyConsoleWidget::OnClickedClose()
{
	CloseWidget();
}

void UNSPartyConsoleWidget::HandleInviteCodeReady(const FString& InviteCode)
{
	CurrentInviteCode = InviteCode;
	if (InviteCodeText)
	{
		InviteCodeText->SetText(FText::FromString(InviteCode));
	}
}

void UNSPartyConsoleWidget::OnClickedCopyCode()
{
	if (!CurrentInviteCode.IsEmpty())
		FPlatformApplicationMisc::ClipboardCopy(*CurrentInviteCode);
}
