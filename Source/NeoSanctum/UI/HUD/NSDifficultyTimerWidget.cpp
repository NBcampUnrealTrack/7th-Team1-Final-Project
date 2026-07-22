// Copyright 2026 One Team. All rights reserved.


#include "NSDifficultyTimerWidget.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"

#define LOCTEXT_NAMESPACE "DifficultyTimerWidget"

void UNSDifficultyTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindRunGameState();
	RefreshDifficultyTimer();
}

void UNSDifficultyTimerWidget::NativeDestruct()
{
	UnBindRunGameState();
	
	Super::NativeDestruct();
}

void UNSDifficultyTimerWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	BindRunGameState();
	RefreshDifficultyTimer();
}

void UNSDifficultyTimerWidget::RefreshDifficultyTimer()
{
	const UWorld* World = GetWorld();
	const ANSRunGameState* RunGameState =
		World ? World->GetGameState<ANSRunGameState>() : nullptr;

	const UNSGameFlowSubsystem* GameFlowSubsystem =
		GetGameFlowSubsystem();

	if (!RunGameState || !GameFlowSubsystem || !RunGameState->ShouldShowDifficultyTimer())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	SetVisibility(ESlateVisibility::HitTestInvisible);
    
	const float Interval = GameFlowSubsystem->GetDifficultyTimeStepInterval();

	if (DifficultyProgressBar)
	{
		DifficultyProgressBar->SetPercent(
			RunGameState->GetDifficultyProgressPercent(Interval));
	}

	if (DifficultyLevelText)
	{
		DifficultyLevelText->SetText(
			FText::AsNumber(
				RunGameState->GetDifficultyLevel(Interval)));
	}

	if (StageNumberText)
	{
		StageNumberText->SetText(
			FText::AsNumber(
				RunGameState->GetDifficultyStageNumber()));
	}
}

void UNSDifficultyTimerWidget::BindRunGameState()
{
	const UWorld* World = GetWorld();
	ANSRunGameState* RunGameState =
		World ? World->GetGameState<ANSRunGameState>() : nullptr;
	
	if (!RunGameState || CachedRunGameState == RunGameState)
	{
		return;
	}
	
	UnBindRunGameState();
	
	CachedRunGameState = RunGameState;
	CachedRunGameState->OnDifficultyTimerStateChanged.AddUObject(
		this,
		&UNSDifficultyTimerWidget::RefreshDifficultyTimer);
}

void UNSDifficultyTimerWidget::UnBindRunGameState()
{
	if (CachedRunGameState)
	{
		CachedRunGameState->OnDifficultyTimerStateChanged.RemoveAll(this);
		CachedRunGameState = nullptr;
	}
}

UNSGameFlowSubsystem* UNSDifficultyTimerWidget::GetGameFlowSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNSGameFlowSubsystem>();
}

#undef LOCTEXT_NAMESPACE