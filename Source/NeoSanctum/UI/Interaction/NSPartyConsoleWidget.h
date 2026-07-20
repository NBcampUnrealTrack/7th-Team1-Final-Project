// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSNPCInteractionWidgetBase.h"
#include "NSPartyConsoleWidget.generated.h"


class UCommonButtonBase;
class UTextBlock;
class UEditableTextBox;

/**
 *  스팀 세션(친구 초대, 참여 등)을 처리할 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSPartyConsoleWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()
	
public:
	// 진입점
	virtual void OpenForInteractor(APlayerController* Interactor) override;
	// 닫기 시 호출
	virtual void OnCloseWidget() override;

protected:
	// 세션 생성
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> CreateSessionButton;

	// 닫기
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> CloseButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InviteCodeText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CopyCodeButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> CodeInputBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> JoinByCodeButton;
	// 친구 목록을 담을 컨테이너
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPanelWidget> FriendListContainer;
	// 엔트리 위젯 클래스 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Party|Friends")
	TSubclassOf<class UNSFriendEntryWidget> FriendEntryClass;

	UFUNCTION()
	void OnClickedCreateSession();
	UFUNCTION()
	void OnClickedClose();
	UFUNCTION()
	void HandleInviteCodeReady(const FString& InviteCode);
	UFUNCTION()
	void OnClickedCopyCode();
	UFUNCTION()
	void OnClickedJoinByCode();
	
	// 친구 목록 갱신 콜백
	UFUNCTION()
	void HandleFriendsListUpdated();
	
	// 목록 UI 갱신
	void RefreshFriendList();

private:
	TWeakObjectPtr<APlayerController> OwningPC;
	
	// HandleInviteCodeReady에서 보관 (코드 복사용)
	FString CurrentInviteCode;
	
	
};
