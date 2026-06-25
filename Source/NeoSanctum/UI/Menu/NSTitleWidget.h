// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "NSTitleWidget.generated.h"

class UCommonButtonBase;
class UEditableTextBox;
class UTextBlock;
class UWidget;


/**
 *  게임 시작 전 표시되는 메인 타이틀 UI
 */
UCLASS()
class NEOSANCTUM_API UNSTitleWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
public:
	//방생성 버튼 클릭
	UFUNCTION()
	void OnClickedHostButton();
	//방참여 버튼 클릭
	UFUNCTION()
	void OnClickedJoinButton();
	//설정 버튼 클릭
	UFUNCTION()
	void OnClickedOptionButton();
	//게임종료 버튼 클릭
	UFUNCTION()
	void OnClickedQuitButton();
	//참가 확인 버튼 클릭 처리
	UFUNCTION()
	void OnClickedConfirmJoinButton();
	//참가 취소 버튼 클릭 처리
	UFUNCTION()
	void OnClickedCancelJoinButton();
	//IP입력창
	UFUNCTION()
	void OnChangedIPText(const FText& ChangedText);
	
private:
	//방생성 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> HostButton;
	//방참여 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> JoinButton;
	//설정 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> OptionButton;
	//게임 종료 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> QuitButton;
	//IP 입력 패널
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> JoinPanel;
	//IP 주소 입력창
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> IPTextBox;
	//참가 확인 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ConfirmJoinButton;
	//참가 취소 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> CancelJoinButton;


	
protected:
	virtual void NativeConstruct() override;
};
