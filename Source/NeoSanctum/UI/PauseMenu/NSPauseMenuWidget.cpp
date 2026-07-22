// Copyright 2026 One Team. All rights reserved.


#include "NSPauseMenuWidget.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"

void UNSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UE_LOG(LogTemp, Warning, TEXT("[Pause] Option=%d Main=%d Quit=%d"),
	OptionButton != nullptr, MainMenuButton != nullptr, QuitButton != nullptr);
	
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
	
	if (CloseButton)
	{
		CloseButton->OnClicked().AddUObject(
			this,
			&UNSPauseMenuWidget::OnCloseClicked);
	}
}

void UNSPauseMenuWidget::NativeDestruct()
{
	if (OptionButton)
	{
		OptionButton->OnClicked().RemoveAll(this);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked().RemoveAll(this);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked().RemoveAll(this);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

void UNSPauseMenuWidget::OnOptionClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[Pause] Option clicked"));
	
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

void UNSPauseMenuWidget::OnCloseClicked()
{
	if (ANSPlayerController* PlayerController =
	Cast<ANSPlayerController>(GetOwningPlayer()))
	{
		PlayerController->TogglePauseMenu();
	}
}
