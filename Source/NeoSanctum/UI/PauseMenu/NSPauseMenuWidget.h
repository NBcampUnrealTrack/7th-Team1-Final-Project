// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPauseMenuWidget.generated.h"

class UNSButtonBase;


UCLASS()
class NEOSANCTUM_API UNSPauseMenuWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase>  OptionButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase>  MainMenuButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase>  QuitButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNSButtonBase>  CloseButton;

	UFUNCTION()
	void OnOptionClicked();
	UFUNCTION()
	void OnMainMenuClicked();
	UFUNCTION()
	void OnQuitClicked();
	UFUNCTION()
	void OnCloseClicked();
};
