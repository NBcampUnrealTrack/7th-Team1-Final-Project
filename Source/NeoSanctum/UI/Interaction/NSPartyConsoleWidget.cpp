// Copyright 2026 One Team. All rights reserved.


#include "NSPartyConsoleWidget.h"
#include "CommonButtonBase.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"


void UNSPartyConsoleWidget::OpenForInteractor(APlayerController* Interactor)
{
	OwningPC = Interactor;

	AddToViewport();

	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked().AddUObject(
			this, &UNSPartyConsoleWidget::OnClickedCreateSession);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this, &UNSPartyConsoleWidget::OnClickedClose);
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
