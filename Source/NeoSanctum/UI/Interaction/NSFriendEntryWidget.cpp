// Copyright 2026 One Team. All rights reserved.


#include "NSFriendEntryWidget.h"
#include "Components/TextBlock.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"

void UNSFriendEntryWidget::Setup(const FNSFriendInfo& InFriendInfo)
{
	FriendNetIdString = InFriendInfo.NetIdString;

	if (FriendNameText)
		FriendNameText->SetText(FText::FromString(InFriendInfo.DisplayName));

	if (StatusText)
		StatusText->SetText(FText::FromString(
			InFriendInfo.bIsOnline ? TEXT("온라인") : TEXT("오프라인")));

	// 온라인인 친구만 초대 가능
	if (InviteButton)
	{
		InviteButton->SetIsEnabled(InFriendInfo.bIsOnline);
		InviteButton->OnClicked().AddUObject(this, &UNSFriendEntryWidget::OnClickedInvite);
	}
}

void UNSFriendEntryWidget::OnClickedInvite()
{
	UE_LOG(LogTemp, Log, TEXT("초대 버튼 클릭: %s"), *FriendNetIdString);
	
	if (UNSSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UNSSessionSubsystem>())
	{
		Session->InviteFriendToSession(FriendNetIdString);
	}
}
