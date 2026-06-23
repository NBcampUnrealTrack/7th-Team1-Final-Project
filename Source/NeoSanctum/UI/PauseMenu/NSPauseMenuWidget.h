// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSPauseMenuWidget.generated.h"

class UCommonButtonBase;


UCLASS()
class NEOSANCTUM_API UNSPauseMenuWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase>  OptionButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase>  MainMenuButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase>  QuitButton;

	UFUNCTION()
	void OnOptionClicked();
	UFUNCTION()
	void OnMainMenuClicked();
	UFUNCTION()
	void OnQuitClicked();
};
