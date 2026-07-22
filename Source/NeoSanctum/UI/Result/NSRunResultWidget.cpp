// Copyright 2026 One Team. All rights reserved.


#include "NSRunResultWidget.h"
#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "GameFramework/GameStateBase.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NSVoterEntry.h"
#include "Components/Image.h"
#include "Components/Widget.h"

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
	
	if (StageInfoText)
	{
		int32 StageNumber = 0;
		if (const UWorld* World = GetWorld())
		{
			if (const ANSRunGameState* RunGameState =
				World->GetGameState<ANSRunGameState>())
			{
				StageNumber = RunGameState->GetDifficultyStageNumber();
			}
		}

		StageInfoText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "StageInfoFormat", "스테이지 {0}"),
			FText::AsNumber(StageNumber)));
	}
	
	if (EarnedGoodsText)
	{
		EarnedGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "EarnedGoodsFormat", "보급 코인 : {0}"),
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
		const APlayerController* PC = GetOwningPlayer();
		const ANSPlayerState* LocalPS = PC ? PC->GetPlayerState<ANSPlayerState>() : nullptr;
		const int32 MyNormalKills = LocalPS ? LocalPS->GetNormalKillCount() : 0;
		
		KillCountText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "KillCountFormat", "처치 수 : {0}"),
			FText::AsNumber(MyNormalKills)));
	}
	
	if (CommonGoodsText)
	{
		CommonGoodsText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "CommonGoodsFormat", "네오 크리스털 : {0}"),
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
	
	if (DamageText || AccuracyText)
	{
		const APlayerController* PC = GetOwningPlayer();
		const ANSPlayerState* LocalPS = PC ? PC->GetPlayerState<ANSPlayerState>() : nullptr;

		// 가한 피해
		if (DamageText)
		{
			const int64 MyDamage = LocalPS ? LocalPS->GetTotalDamageDealt() : 0;
			DamageText->SetText(FText::Format(
				NSLOCTEXT("RunResult", "DamageFormat", "가한 피해량 : {0}"),
				FText::AsNumber(MyDamage)));
		}

		// 명중률
		if (AccuracyText)
		{
			const int32 Fired = LocalPS ? LocalPS->GetShotsFired() : 0;
			const int32 Hit   = LocalPS ? LocalPS->GetShotsHit()   : 0;

			const float AccuracyPercent =
				(Fired > 0) ? (static_cast<float>(Hit) / Fired) * 100.0f : 0.0f;

			FNumberFormattingOptions NumberFormat;
			NumberFormat.MinimumFractionalDigits = 1;
			NumberFormat.MaximumFractionalDigits = 1;

			AccuracyText->SetText(FText::Format(
				NSLOCTEXT("RunResult", "AccuracyFormat", "주무기 명중률 : {0}%"),
				FText::AsNumber(AccuracyPercent, &NumberFormat)));
		}
	}
	
	if (NextVotePanel)
	{
		NextVotePanel->SetVisibility(
			bCleared
				? ESlateVisibility::Visible
				: ESlateVisibility::Hidden);
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
	UpdateVoteButtonState(); 
}

void UNSRunResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (RunEndButtonStyle)
	{
		if (NextStageButton)
		{
			NextStageButton->SetStyle(RunEndButtonStyle);
		}

		if (ReturnToHubButton)
		{
			ReturnToHubButton->SetStyle(RunEndButtonStyle);
		}
	}
	
	NextRows = { NextRow0, NextRow1, NextRow2, NextRow3 };
	HubRows  = { HubRow0,  HubRow1,  HubRow2,  HubRow3  };
	VoteImages = { VoteImage0, VoteImage1, VoteImage2, VoteImage3 };
	
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
	if (!World) { return; }

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState) { return; }

	// 투표자 이름 두 그룹으로 수집
	TArray<FString> NextNames;
	TArray<FString> HubNames;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState || !NSPlayerState->bVoteConfirmed) { continue; }

		const FString PlayerName = NSPlayerState->GetPlayerName();

		if (NSPlayerState->RunChoice == ENSRunChoice::NextStage)
		{
			NextNames.Add(PlayerName);
		}
		else if (NSPlayerState->RunChoice == ENSRunChoice::ReturnToHub)
		{
			HubNames.Add(PlayerName);
		}
	}

	// 표시 순서 안정화
	NextNames.Sort();
	HubNames.Sort();

	// 위에서부터 채우고 남는 칸은 비움
	auto FillRows = [](const TArray<TObjectPtr<UNSVoterEntry>>& Rows,
					   const TArray<FString>& Names)
	{
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			if (!Rows[i]) { continue; }

			if (i < Names.Num()) { Rows[i]->SetVoter(Names[i]); }
			else                 { Rows[i]->ClearVoter(); }
		}
	};

	FillRows(NextRows, NextNames);
	FillRows(HubRows,  HubNames);
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
	RefreshVoteSummary();
	RefreshLocalVoteSelection();
}

void UNSRunResultWidget::RefreshVoteSummary()
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

	// 분자: 투표 확정 인원
	int32 VotedCount = 0;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		const ANSPlayerState* NSPS = Cast<ANSPlayerState>(PS);
		if (NSPS && NSPS->bVoteConfirmed)
		{
			++VotedCount;
		}
	}

	// 분모: 접속 인원
	const int32 TotalCount = GameState->PlayerArray.Num();

	// 텍스트 ex) 2 / 4
	if (VoteCountText)
	{
		VoteCountText->SetText(FText::Format(
			NSLOCTEXT("RunResult", "VoteCountFormat", "투표 참여 {0} / {1}"),
			FText::AsNumber(VotedCount),
			FText::AsNumber(TotalCount)));
	}

	// 투표한 수만큼만 아이콘을 보이게
	for (int32 i = 0; i < VoteImages.Num(); ++i)
	{
		if (!VoteImages[i])
		{
			continue;
		}

		const bool bShow = (i < VotedCount);
		VoteImages[i]->SetVisibility(
			bShow
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
	}
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
	// 현재 페이즈 확인
	bool bIsVotingPhase = false;
	if (const UWorld* World = GetWorld())
	{
		if (const ANSRunGameState* RunGameState = World->GetGameState<ANSRunGameState>())
		{
			bIsVotingPhase = (RunGameState->RunEndPhase == ENSRunEndPhase::Voting);
		}
	}

	// 투표 페이즈가 아니면 무조건 둘 다 잠금
	if (!bIsVotingPhase)
	{
		if (NextStageButton)   { NextStageButton->SetIsEnabled(false); }
		if (ReturnToHubButton) { ReturnToHubButton->SetIsEnabled(false); }
		return;
	}

	// Voting 페이즈일 때만 자기표 로직
	if (NextStageButton)
	{
		const bool bCanSelectNext =
			bLastRunCleared &&
			(!bHasSelectedChoice || SelectedChoice != ENSRunChoice::NextStage);
		NextStageButton->SetIsEnabled(bCanSelectNext);
	}

	if (ReturnToHubButton)
	{
		const bool bCanSelectHub =
			!bHasSelectedChoice || SelectedChoice != ENSRunChoice::ReturnToHub;
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
