// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Core/GameFlow/NSSessionType.h"
#include "NSFriendEntryWidget.generated.h"

class UTextBlock;
class UCommonButtonBase;
class UImage;

UCLASS()
class NEOSANCTUM_API UNSFriendEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	// 친구 정보로 위젯 세팅(파티 콘솔이 호출)
	void Setup(const FNSFriendInfo& InFriendInfo);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FriendNameText;

	// 온라인 상태 표시 (텍스트로 "온라인"/"오프라인", 나중에 색/아이콘으로 바꿔도 됨)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// 초대 버튼 (온라인일 때만 활성화)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> InviteButton;
	
	// 스팀 프로필 사진
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AvatarImage;  
	
	// SteamID 문자열로 아바타 텍스처를 만들어 반환 (없으면 nullptr)
	UTexture2D* BuildSteamAvatarTexture(const FString& SteamIdString) const;

	UFUNCTION()
	void OnClickedInvite();
	
	// 현재 친구의 아바타를 조회해 AvatarImage에 유효할 때만 적용
	void ApplyAvatar();

private:
	// 이 친구의 NetId (초대 전송용)
	FString FriendNetIdString;
};
