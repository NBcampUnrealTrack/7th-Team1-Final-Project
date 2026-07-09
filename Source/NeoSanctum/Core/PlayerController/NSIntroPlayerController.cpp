// Copyright 2026 One Team. All rights reserved.


#include "NSIntroPlayerController.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaSoundComponent.h" 
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "MoviePlayer.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Data/Config/NSLevelCatalog.h"
#include "NeoSanctum/UI/Prologue/NSPrologueWidget.h"

ANSIntroPlayerController::ANSIntroPlayerController()
{
	bShowMouseCursor = false;
	
	PrologueMediaSound = 
		CreateDefaultSubobject<UMediaSoundComponent>(TEXT("PrologueMediaSound"));
}

void ANSIntroPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (PrologueWidgetClass && !PrologueWidget)
	{
		PrologueWidget = CreateWidget<UNSPrologueWidget>(this, PrologueWidgetClass);
		if (PrologueWidget) PrologueWidget->AddToViewport(500);
	}
	
	SetInputMode(FInputModeGameOnly());

	// 프롤로그와 병행하는 데이터 선로딩
	StartPreload(); 

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IntroMappingContext)
			{
				Sub->AddMappingContext(IntroMappingContext, 0);
			}
		}
	}

	StartPrologue();
}

void ANSIntroPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SkipAction)
		{
			EIC->BindAction(
				SkipAction,
				ETriggerEvent::Ongoing, 
				this,
				&ThisClass::OnSkipOngoing);
			EIC->BindAction(
				SkipAction,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::OnSkipTriggered);
			EIC->BindAction(
				SkipAction,
				ETriggerEvent::Canceled,
				this,
				&ThisClass::OnSkipReleased);
			EIC->BindAction(
				SkipAction, 
				ETriggerEvent::Completed, 
				this,
				&ThisClass::OnSkipReleased);
		}
	}
}

void ANSIntroPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (PrologueMediaPlayer)
	{
		PrologueMediaPlayer->OnEndReached.RemoveDynamic(
			this, 
			&ThisClass::HandleMediaEnd);
	}
	Super::EndPlay(Reason);
}

void ANSIntroPlayerController::StartPrologue()
{
	if (PrologueWidgetClass)
	{
		PrologueWidget = CreateWidget<UNSPrologueWidget>(this, PrologueWidgetClass);
		if (PrologueWidget) PrologueWidget->AddToViewport(500);
	}

	if (PrologueMediaPlayer && PrologueMediaSource)
	{
		// 재생 종료 바인딩
		PrologueMediaPlayer->OnEndReached.AddDynamic(this, &ThisClass::HandleMediaEnd);

		if (PrologueMediaSound && PrologueMediaPlayer)
		{
			PrologueMediaSound->SetMediaPlayer(PrologueMediaPlayer);
			PrologueMediaSound->Activate(true);
		}

		PrologueMediaPlayer->SetLooping(false);
		PrologueMediaPlayer->OpenSource(PrologueMediaSource);
	}
	else
	{
		FinishPrologue();
	}
}

void ANSIntroPlayerController::StartPreload()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->PreloadOutGameData(); 
	}
}

void ANSIntroPlayerController::FinishPrologue()
{
	if (bFinishing)
	{
		return;
	}
	
	bFinishing = true;

	if (PrologueMediaPlayer)
	{
		PrologueMediaPlayer->OnEndReached.RemoveDynamic(
			this, 
			&ThisClass::HandleMediaEnd);
		
		PrologueMediaPlayer->Close();
	}
	
	if (PrologueWidget)
	{
		PrologueWidget->RemoveFromParent(); PrologueWidget = nullptr;
	}

	FName TitlePackage = NAME_None;
	if (INSGameInstanceInterface* GII = Cast<INSGameInstanceInterface>(GetGameInstance()))
	{
		if (UNSLevelCatalog* Catalog = GII->GetLevelCatalog())
		{
			if (!Catalog->TitleLevel.IsNull())
			{
				TitlePackage = FName(*Catalog->TitleLevel.ToSoftObjectPath().GetLongPackageName());
			}
		}
	}

	if (TitlePackage != NAME_None)
	{
		UGameplayStatics::OpenLevel(this, TitlePackage); 
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Catalog TitleLevel 미설정: 타이틀 이동 실패"));
	}
}

void ANSIntroPlayerController::HandleMediaEnd()
{
	FinishPrologue();
}

void ANSIntroPlayerController::OnSkipOngoing(const FInputActionInstance& Instance)
{
	if (PrologueWidget && SkipHoldSeconds > 0.0f)
	{
		const float Ratio = FMath::Clamp(
			Instance.GetElapsedTime() / SkipHoldSeconds,
			0.0f,
			1.0f);
		PrologueWidget->SetSkipProgress(Ratio);
	}
}

void ANSIntroPlayerController::OnSkipTriggered(const FInputActionInstance& Instance)
{
	FinishPrologue();
}

void ANSIntroPlayerController::OnSkipReleased(const FInputActionInstance& Instance)
{
	if (PrologueWidget)
	{
		PrologueWidget->SetSkipProgress(0.0f);
	}
}
