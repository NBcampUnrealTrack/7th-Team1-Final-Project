// Copyright 2026 One Team. All rights reserved.


#include "NSDifficultyTimerWidget.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSGameFlowSubsystem.h"

#define LOCTEXT_NAMESPACE "DifficultyTimerWidget"

void UNSDifficultyTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshDifficultyTimer();
}

void UNSDifficultyTimerWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UNSDifficultyTimerWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshDifficultyTimer();
}

void UNSDifficultyTimerWidget::RefreshDifficultyTimer()
{
	const UNSGameFlowSubsystem* GameFlowSubsystem =
		GetGameFlowSubsystem();
		
	if (!GameFlowSubsystem || !GameFlowSubsystem->ShouldShowDifficultyTimer())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
    
	if (DifficultyProgressBar)
	{
		DifficultyProgressBar->SetPercent(
			GameFlowSubsystem->GetDifficultyProgressPercent());
	}

	if (DifficultyLevelText)
	{
		DifficultyLevelText->SetText(
			FText::AsNumber(
				GameFlowSubsystem->GetDifficultyLevel()));
	}

	if (StageNumberText)
	{
		StageNumberText->SetText(
			FText::AsNumber(
				GameFlowSubsystem->GetCurrentStageNumber()));
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