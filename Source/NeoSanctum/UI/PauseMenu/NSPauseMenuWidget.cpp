// Copyright 2026 One Team. All rights reserved.


#include "NSPauseMenuWidget.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"

void UNSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (OptionButton)
	{
		OptionButton->OnClicked().AddUObject(this, &UNSPauseMenuWidget::OnOptionClicked);
	}
	
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked().AddUObject(this, &UNSPauseMenuWidget::OnMainMenuClicked);
	}
	
	if (QuitButton)
	{
		QuitButton->OnClicked().AddUObject(this, &UNSPauseMenuWidget::OnQuitClicked);
	}
}

void UNSPauseMenuWidget::OnOptionClicked()
{
	UGameInstance* GameInstance = GetGameInstance();
	UNSUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (UIManager)
	{
		UIManager->OpenOptionPanel(GetOwningPlayer());
	}
}

void UNSPauseMenuWidget::OnMainMenuClicked()
{
	if (ANSPlayerController* PC = Cast<ANSPlayerController>(GetOwningPlayer()))
	{
		// 세션 정리 + 타이틀복귀
		PC->RequestLeaveToMainMenu();  
	}
}

void UNSPauseMenuWidget::OnQuitClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(
			this,
			PC, 
			EQuitPreference::Quit, 
			false);
	}
}