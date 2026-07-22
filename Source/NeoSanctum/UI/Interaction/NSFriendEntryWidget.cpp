// Copyright 2026 One Team. All rights reserved.


#include "NSFriendEntryWidget.h"
#include "Components/TextBlock.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "steam/steam_api.h"

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
		InviteButton->OnClicked().RemoveAll(this);
		InviteButton->OnClicked().AddUObject(this, &UNSFriendEntryWidget::OnClickedInvite);
	}
	
	ApplyAvatar(); 
}

UTexture2D* UNSFriendEntryWidget::BuildSteamAvatarTexture(const FString& SteamIdString) const
{
	if (!SteamFriends() || !SteamUtils())
	{
		return nullptr;
	}

	const uint64 SteamId64 = FCString::Strtoui64(*SteamIdString, nullptr, 10);
	if (SteamId64 == 0)
	{
		return nullptr;
	}
	const CSteamID FriendSteamId(SteamId64);

	const int AvatarHandle = SteamFriends()->GetMediumFriendAvatar(FriendSteamId);
	if (AvatarHandle <= 0)
	{
		// 0 = 아바타 없음, -1 = 아직 다운로드 안 됨
		return nullptr;
	}

	uint32 Width = 0;
	uint32 Height = 0;
	if (!SteamUtils()->GetImageSize(AvatarHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		return nullptr;
	}

	const int32 BufferSize = Width * Height * 4;
	TArray<uint8> AvatarRGBA;
	AvatarRGBA.SetNumUninitialized(BufferSize);
	if (!SteamUtils()->GetImageRGBA(AvatarHandle, AvatarRGBA.GetData(), BufferSize))
	{
		return nullptr;
	}

	UTexture2D* AvatarTexture =
		UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!AvatarTexture)
	{
		return nullptr;
	}

	FTexture2DMipMap& Mip = AvatarTexture->GetPlatformData()->Mips[0];
	void* MipData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, AvatarRGBA.GetData(), BufferSize);
	Mip.BulkData.Unlock();

	AvatarTexture->UpdateResource();
	return AvatarTexture;
}

void UNSFriendEntryWidget::ApplyAvatar()
{
	if (!AvatarImage)
	{
		return;
	}

	if (UTexture2D* Avatar = BuildSteamAvatarTexture(FriendNetIdString))
	{
		AvatarImage->SetBrushFromTexture(Avatar);
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
