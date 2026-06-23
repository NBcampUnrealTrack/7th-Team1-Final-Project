// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSReadyStartWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;

/**
 * 거점 Ready/Start 상호작용 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSReadyStartWidget : public UCommonUserWidget
{
	GENERATED_BODY()

	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> StartButton;
	
	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonButtonBase> CloseButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ReadyButtonText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StartButtonText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ReadyStatusText;
	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void CloseWidget();
	
	void RefreshReadyButtonText();
	
	void InitializeButtonText();
	
	// 버튼 텍스트는 로컬 입력 즉시 반응해야 하므로,
	// 서버 복제 전에도 현재 플레이어의 예상 Ready 상태를 들고 있다.
	bool bLocalReadySelected = false;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};