// Copyright 2026 One Team. All rights reserved.


#include "NSPartyConsoleWidget.h"
#include "CommonButtonBase.h"
#include "NSFriendEntryWidget.h"
#include "Components/EditableTextBox.h"
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
	if (JoinByCodeButton)
	{
		JoinByCodeButton->OnClicked().AddUObject(
			this,
		 	&UNSPartyConsoleWidget::OnClickedJoinByCode);
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
		
		// 친구 목록 갱신 구독과 요청
		Session->OnFriendsListUpdated.RemoveDynamic(
			this, 
			&UNSPartyConsoleWidget::HandleFriendsListUpdated);
		Session->OnFriendsListUpdated.AddDynamic(
			this,
			&UNSPartyConsoleWidget::HandleFriendsListUpdated);
		Session->RequestFriendsList();
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

void UNSPartyConsoleWidget::OnCloseWidget()
{
	if (UNSSessionSubsystem* Session =
		GetGameInstance() ?
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr)
	{
		Session->OnInviteCodeReady.RemoveDynamic(
			this,
			&UNSPartyConsoleWidget::HandleInviteCodeReady);
		Session->OnFriendsListUpdated.RemoveDynamic(
			this,
			&UNSPartyConsoleWidget::HandleFriendsListUpdated);
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

void UNSPartyConsoleWidget::OnClickedJoinByCode()
{
	if (!CodeInputBox)
	{
		return;
	}

	const FString EnteredCode = CodeInputBox->GetText().ToString().TrimStartAndEnd();
	if (EnteredCode.IsEmpty())
	{
		return;
	}

	if (UNSSessionSubsystem* Session =
		GetGameInstance()->GetSubsystem<UNSSessionSubsystem>())
	{
		Session->JoinSessionByCode(EnteredCode);
	}
}

void UNSPartyConsoleWidget::HandleFriendsListUpdated()
{
	RefreshFriendList();
}

void UNSPartyConsoleWidget::RefreshFriendList()
{
	if (!FriendListContainer || !FriendEntryClass)
	{
		return;
	}

	UNSSessionSubsystem* Session =
		GetGameInstance() ? 
	GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr;
	if (!Session)
	{
		return;
	}

	// 기존 항목 비우기
	FriendListContainer->ClearChildren();

	// 캐시된 친구 목록 꺼내기
	TArray<FNSFriendInfo> Friends;
	Session->GetCachedFriends(Friends);

	for (const FNSFriendInfo& FriendInfo : Friends)
	{
		UNSFriendEntryWidget* Entry =
			CreateWidget<UNSFriendEntryWidget>(this, FriendEntryClass);
		if (Entry)
		{
			Entry->Setup(FriendInfo);
			FriendListContainer->AddChild(Entry);
		}
	}
}
