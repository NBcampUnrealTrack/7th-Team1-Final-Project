// Copyright 2026 One Team. All rights reserved.


#include "NSRunResultWidget.h"
#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "CommonButtonBase.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"

void UNSRunResultWidget::SetRunResult(
	bool bCleared,
	int32 EarnedGoods,
	int32 CommonGoods,
	float RunTimeSeconds,
	int32 KillCount)
{
	bLastRunCleared = bCleared;
	
	if (ResultTitleText)
	{
		ResultTitleText->SetText(
			bCleared
				? NSLOCTEXT("RunResult", "ClearTitle", "작전 성공")
				: NSLOCTEXT("RunResult", "FailedTitle", "작전 실패"));
	}
	
	if (EarnedGoodsText)
	{
		EarnedGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "EarnedGoodsFormat", "임시 재화 : {0}"),
			FText::AsNumber(EarnedGoods)));
	}
	
	if (RunTimeText)
	{
		RunTimeText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "RunTimeFormat", "진행 시간 : {0}"),
			FormatRunTime(RunTimeSeconds)));
	}
	
	if (KillCountText)
	{
		KillCountText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "KillCountFormat", "처치 수 : {0}"),
			FText::AsNumber(KillCount)));
	}
	
	if (CommonGoodsText)
	{
		CommonGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "CommonGoodsFormat", "영구 재화 : {0}"),
			FText::AsNumber(CommonGoods)));
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
	
	if (NextVotersText)
	{
		NextVotersText->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	
	if (HubVotersText)
	{
		HubVotersText->SetVisibility(ESlateVisibility::Visible);
	}
	
	RefreshLocalVoteSelection();
}

void UNSRunResultWidget::SetVoteResult(int32 NextVotes, int32 HubVotes)
{
	if (NextVotesText)
	{
		NextVotesText->SetText(FText::Format(
			NSLOCTEXT(
				"RunResult",
				"NextVotesFormat",
				"다음 스테이지 : {0}"),
			FText::AsNumber(NextVotes)));
	}

	if (HubVotesText)
	{
		HubVotesText->SetText(FText::Format(
			NSLOCTEXT(
				"RunResult",
				"HubVotesFormat",
				"거점 복귀 : {0}"),
			FText::AsNumber(HubVotes)));
	}
}

void UNSRunResultWidget::HandleNextStageClicked()
{
	SubmitVote(ENSRunChoice::NextStage);
}

void UNSRunResultWidget::HandleReturnToHubClicked()
{
	SubmitVote(ENSRunChoice::ReturnToHub);
}

void UNSRunResultWidget::SetSelectedChoice(ENSRunChoice NewChoice)
{
	SelectedChoice = NewChoice;
	bHasSelectedChoice = true;
	UpdateVoteButtonState();
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
	SetRunResult(false, 0, 0, 0.0f, 0);
}

void UNSRunResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetRunResult(false, 0, 0, 0.0f, 0);
	
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
	
	BindRunEndVoteChanged();
	RefreshVoteInfo();
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
	
	UnbindRunEndVoteChanged();
	
	Super::NativeDestruct();
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

void UNSRunResultWidget::RefreshVoteVoters()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}
	
	FString NextVoters =
		NSLOCTEXT(
			"RunResult",
			"NextStageVotersTitle",
			"다음 스테이지\n").ToString();

	FString HubVoters =
		NSLOCTEXT(
			"RunResult",
			"HubVotersTitle",
			"거점 복귀\n").ToString();
	
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState || !NSPlayerState->bVoteConfirmed)
		{
			continue;
		}
		
		const FString PlayerName = NSPlayerState->GetPlayerName();
		
		if (NSPlayerState->RunChoice == ENSRunChoice::NextStage)
		{
			NextVoters += FString::Printf(TEXT("%s\n"), *PlayerName);
		}
		else if (NSPlayerState->RunChoice == ENSRunChoice::ReturnToHub)
		{
			HubVoters += FString::Printf(TEXT("%s\n"), *PlayerName);
		}
	}
	
	if (NextVotersText)
	{
		NextVotersText->SetText(FText::FromString(NextVoters));
		NextVotersText->SetVisibility(
			bLastRunCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	
	if (HubVotersText)
	{
		HubVotersText->SetText(FText::FromString(HubVoters));
		HubVotersText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UNSRunResultWidget::RefreshVoteInfo()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	ANSRunGameState* RunGameState =
		World->GetGameState<ANSRunGameState>();
	
	if (!RunGameState)
	{
		return;
	}
	
	SetVoteResult(RunGameState->NextVotes, RunGameState->HubVotes);
	RefreshVoteVoters();
	RefreshLocalVoteSelection();
}

void UNSRunResultWidget::BindRunEndVoteChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	ANSRunGameState* RunGameState = 
		World->GetGameState<ANSRunGameState>();
	
	if (!RunGameState)
	{
		return;
	}
	
	RunGameState->OnRunEndVoteChanged.RemoveDynamic(
		this,
		&UNSRunResultWidget::RefreshVoteInfo);
	
	RunGameState->OnRunEndVoteChanged.AddDynamic(
		this,
		&UNSRunResultWidget::RefreshVoteInfo);
}

void UNSRunResultWidget::UnbindRunEndVoteChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	ANSRunGameState* RunGameState =
		World->GetGameState<ANSRunGameState>();
	if (!RunGameState)
	{
		return;
	}
	
	RunGameState->OnRunEndVoteChanged.RemoveDynamic(
		this,
		&UNSRunResultWidget::RefreshVoteInfo);
}
void UNSRunResultWidget::SubmitVote(ENSRunChoice NewChoice)
{
	ANSPlayerController* PlayerController =
		Cast<ANSPlayerController>(GetOwningPlayer());

	if (!PlayerController)
	{
		return;
	}

	// 실패 결과에서는 다음 스테이지 투표를 허용하지 않는다.
	if (!bLastRunCleared && NewChoice == ENSRunChoice::NextStage)
	{
		return;
	}

	// 이미 선택한 버튼을 다시 누르는 경우는 무시한다.
	if (bHasSelectedChoice && SelectedChoice == NewChoice)
	{
		return;
	}

	// 서버 응답을 기다리지 않고 로컬 UI를 먼저 갱신한다.
	SetSelectedChoice(NewChoice);

	// 서버에서 기존 표를 새 선택지로 변경한다.
	PlayerController->Server_ConfirmVote(NewChoice);
}

void UNSRunResultWidget::UpdateVoteButtonState()
{
	if (NextStageButton)
	{
		const bool bCanSelectNext =
			bLastRunCleared &&
			(!bHasSelectedChoice ||
				SelectedChoice != ENSRunChoice::NextStage);

		NextStageButton->SetIsEnabled(bCanSelectNext);
	}

	if (ReturnToHubButton)
	{
		const bool bCanSelectHub =
			!bHasSelectedChoice ||
			SelectedChoice != ENSRunChoice::ReturnToHub;

		ReturnToHubButton->SetIsEnabled(bCanSelectHub);
	}
}

void UNSRunResultWidget::RefreshLocalVoteSelection()
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const ANSPlayerState* PlayerState = PlayerController
		? PlayerController->GetPlayerState<ANSPlayerState>()
		: nullptr;

	bHasSelectedChoice =
		PlayerState && PlayerState->bVoteConfirmed;

	if (bHasSelectedChoice)
	{
		SelectedChoice = PlayerState->RunChoice;
	}

	UpdateVoteButtonState();
}
