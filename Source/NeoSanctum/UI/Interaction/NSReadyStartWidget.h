// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSReadyStartWidget.generated.h"

class UNSButtonBase;
class UCommonTextBlock;
class UImage;
class UNSReadyPlayerEntry; 
class UEditableTextBox; 

/**
 * 거점 Ready/Start 상호작용 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSReadyStartWidget : public UCommonUserWidget
{
	GENERATED_BODY()

	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase> StartButton;
	
	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UNSButtonBase> CloseButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ReadyButtonText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StartButtonText;
	
	// 세션 버튼/입력
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CreateSessionButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CopyCodeButton;
	UPROPERTY(meta = (BindWidgetOptional)) 
	TObjectPtr<UCommonButtonBase> JoinByCodeButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UEditableTextBox> CodeInputBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> InviteCodeText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ReadyCountText;
	
	// 친구 목록
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UPanelWidget> FriendListContainer;
	UPROPERTY(EditDefaultsOnly, Category = "Party|Friends")
	TSubclassOf<class UNSFriendEntryWidget> FriendEntryClass;
	
	// 친구 검색창
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> FriendSearchBox;
	
	// 레디 아이콘 4개
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReadyImage0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReadyImage1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReadyImage2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ReadyImage3;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSReadyPlayerEntry> PlayerRow0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSReadyPlayerEntry> PlayerRow1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNSReadyPlayerEntry> PlayerRow2;
	UPROPERTY(meta = (BindWidgetOptional)) 
	TObjectPtr<UNSReadyPlayerEntry> PlayerRow3;
	
	UPROPERTY()
	TArray<TObjectPtr<UImage>> ReadyImages;
	
	UPROPERTY() TArray<TObjectPtr<UNSReadyPlayerEntry>>
	PlayerRows;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ready|Icon")
	TObjectPtr<UObject> ReadyActiveSprite;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ready|Icon")
	TObjectPtr<UObject> ReadyDefaultSprite;

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void CloseWidget();
	
	void RefreshReadyButtonText();
	
	void InitializeButtonText();
	
	// 핸들러
	UFUNCTION()
	void OnClickedCreateSession();
	UFUNCTION()
	void HandleInviteCodeReady(const FString& InviteCode);
	UFUNCTION()
	void OnClickedCopyCode();
	UFUNCTION()
	void OnClickedJoinByCode();
	UFUNCTION()
	void HandleFriendsListUpdated();
	void RefreshFriendList();
	
	// 검색어 변경 콜백
	UFUNCTION()
	void OnFriendSearchChanged(const FText& Text);
	
	UFUNCTION()
	void RefreshReadyStatusText();
	// Ready 상태가 변경될 때마다 GameState 델리게이트에 반응해 목록을 갱신한다.
	// Tick/Timer 없이 상태 변경 시점에만 UI를 업데이트하기 위한 바인딩이다.
	void BindReadyStateChanged();
	void UnbindReadyStateChanged();
	
	// 준비 현황 갱신
	void RefreshReadySummary();
	
	// 버튼 텍스트는 로컬 입력 즉시 반응해야 하므로,
	// 서버 복제 전에도 현재 플레이어의 예상 Ready 상태를 들고 있다.
	bool bLocalReadySelected = false;
	
	FString CurrentInviteCode;
	FString CurrentFriendFilter;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};