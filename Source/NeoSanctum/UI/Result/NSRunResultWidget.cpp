// Copyright 2026 One Team. All rights reserved.


#include "NSRunResultWidget.h"
#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"

void UNSRunResultWidget::SetRunResult(
bool bCleared,
int32 EarnedGoods,
int32 CommonGoods,
int32 SkillGoods,
float RunTimeSeconds,
int32 KillCount)
{
	if (ResultTitleText)
	{
		ResultTitleText->SetText(
			bCleared
			? NSLOCTEXT("RunResult", "ClearTitle", "Clear")
			: NSLOCTEXT("RunResult", "FailedTitle", "FAILED"));
	}
	
	if (EarnedGoodsText)
	{
		EarnedGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "EarnedGoodsFormat", "Goods : {0}"),
			FText::AsNumber(EarnedGoods)));
	}
	
	if (RunTimeText)
	{
		RunTimeText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "RunTimeFormat", "Time: {0}"),
			FormatRunTime(RunTimeSeconds)));
	}
	
	if (KillCountText)
	{
		KillCountText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "KillCountFormat", "Kills: {0}"),
			FText::AsNumber(KillCount)));
	}
	
	if (CommonGoodsText)
	{
		CommonGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "CommonGoodsFormat", "Common : {0}"),
			FText::AsNumber(CommonGoods)));
	}

	if (SkillGoodsText)
	{
		SkillGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "SkillGoodsFormat", "Skill : {0}"),
			FText::AsNumber(SkillGoods)));
	}
	
	if (NextStageButton)
	{
		NextStageButton->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (NextVotesText)
	{
		NextVotesText->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (!bCleared)
	{
		SetSelectedChoice(ENSRunChoice::ReturnToHub);
	}
	SetVoteSubmitted(false);
}

void UNSRunResultWidget::SetVoteResult(int32 NextVotes, int32 HubVotes)
{
	if (NextVotesText)
	{
		NextVotesText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "NextVotesFormat", "Next : {0}"),
			FText::AsNumber(NextVotes)));
	}

	if (HubVotesText)
	{
		HubVotesText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "HubVotesFormat", "Hub : {0}"),
			FText::AsNumber(HubVotes)));
	}
}

void UNSRunResultWidget::HandleNextStageClicked()
{
	SetSelectedChoice(ENSRunChoice::NextStage);
}

void UNSRunResultWidget::HandleReturnToHubClicked()
{
	SetSelectedChoice(ENSRunChoice::ReturnToHub);
}

void UNSRunResultWidget::HandleConfirmClicked()
{
	ANSPlayerController* PlayerController =
		Cast<ANSPlayerController>(GetOwningPlayer());
	if (!PlayerController)
	{
		return;
	}

	if (bVoteSubmitted)
	{
		PlayerController->Server_CancelVote();
		SetVoteSubmitted(false);
		return;
	}

	if (!bHasSelectedChoice)
	{
		return;
	}

	PlayerController->Server_ConfirmVote(SelectedChoice);
	SetVoteSubmitted(true);
}

void UNSRunResultWidget::SetSelectedChoice(ENSRunChoice NewChoice)
{
	SelectedChoice = NewChoice;
	bHasSelectedChoice = true;
}

FText UNSRunResultWidget::FormatRunTime(float RunTimeSeconds) const
{
	const int32 TotalSeconds = FMath::Max(FMath::FloorToInt(RunTimeSeconds), 0);
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	return FText::FromString(FString::Printf(
		TEXT("%02d:%02d"),
		Minutes,
		Seconds));
}
void UNSRunResultWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	//에디터에서 위젯을 열어쓸때 기본표시 상태 확인
	SetRunResult(false,0,0,0,0.0f,0);
}

void UNSRunResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetRunResult(false,0,0,0,0.0f,0);
	SetVoteSubmitted(false);
	
	if (NextStageButton)
	{
		NextStageButton->OnClicked().AddUObject(
			this,
			&UNSRunResultWidget::HandleNextStageClicked);
	}

	if (ReturnToHubButton)
	{
		ReturnToHubButton->OnClicked().AddUObject(
			this,
			&UNSRunResultWidget::HandleReturnToHubClicked);
	}

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked().AddUObject(
			this,
			&UNSRunResultWidget::HandleConfirmClicked);
	}
}

void UNSRunResultWidget::NativeDestruct()
{
	if (NextStageButton)
	{
		NextStageButton->OnClicked().RemoveAll(this);
	}

	if (ReturnToHubButton)
	{
		ReturnToHubButton->OnClicked().RemoveAll(this);
	}

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked().RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

void UNSRunResultWidget::SetVoteSubmitted(bool bSubmitted)
{
	bVoteSubmitted = bSubmitted;

	if (ConfirmButtonText)
	{
		ConfirmButtonText->SetText(
			bVoteSubmitted
				? NSLOCTEXT("RunResult", "CancelVote", "취소")
				: NSLOCTEXT("RunResult", "ConfirmVote", "확인"));
	}
	
	if (NextStageButton)
	{
		NextStageButton->SetIsEnabled(!bVoteSubmitted);
	}

	if (ReturnToHubButton)
	{
		ReturnToHubButton->SetIsEnabled(!bVoteSubmitted);
	}
}
void UNSRunResultWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdatePhaseTimerText();
}
void UNSRunResultWidget::UpdatePhaseTimerText()
{
	if (!TimerText)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		TimerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ANSRunGameState* RunGameState =
		World->GetGameState<ANSRunGameState>();
	if (!RunGameState)
	{
		TimerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float RemainingTime = RunGameState->GetPhaseTimeRemaining();
	const int32 DisplaySeconds = FMath::CeilToInt(RemainingTime);

	TimerText->SetText(FText::Format(
		NSLOCTEXT("RunResult", "TimerFormat", "{0}"),
		FText::AsNumber(DisplaySeconds)));

	TimerText->SetVisibility(ESlateVisibility::Visible);
}