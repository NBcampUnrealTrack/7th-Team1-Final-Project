// Copyright 2026 One Team. All rights reserved.


#include "NSStageObjectiveWidget.h"
#include "CommonTextBlock.h"
#include "Components/Overlay.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"

#define LOCTEXT_NAMESPACE "StageObjectiveWidget"

void UNSStageObjectiveWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BindToRunGameState();
	RefreshStageObjective();
	RefreshBossGate();
}

void UNSStageObjectiveWidget::NativeDestruct()
{
	UnbindFromRunGameState();
	
	Super::NativeDestruct();
}

void UNSStageObjectiveWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (IsValid(CachedRunGameState)
		&&CachedRunGameState->StagePhase
		== ENSStagePhase::BossReady
		&&CachedRunGameState->BossGateEndServerTime > 0.0f)
	{
		UpdateTransitionCountdown();
	}
}

void UNSStageObjectiveWidget::BindToRunGameState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	ANSRunGameState* RunGameState = World->GetGameState<ANSRunGameState>();
	
	if (!RunGameState)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	CachedRunGameState = RunGameState;
	
	RunGameState->OnStageObjectiveChanged.RemoveDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	RunGameState->OnStagePhaseChanged.RemoveDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	RunGameState->OnStageObjectiveChanged.AddDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	RunGameState->OnStagePhaseChanged.AddDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	RunGameState->OnBossGateChanged.RemoveDynamic(
		this,
		&ThisClass::RefreshBossGate);

	RunGameState->OnBossGateChanged.AddDynamic(
		this,
		&ThisClass::RefreshBossGate);
}

void UNSStageObjectiveWidget::UnbindFromRunGameState()
{
	if (!IsValid(CachedRunGameState))
	{
		return;
	}
	
	CachedRunGameState->OnStageObjectiveChanged.RemoveDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	CachedRunGameState->OnStagePhaseChanged.RemoveDynamic(
		this,
		&ThisClass::RefreshStageObjective);
	
	CachedRunGameState->OnBossGateChanged.RemoveDynamic(
	this,
	&ThisClass::RefreshBossGate);
	
	CachedRunGameState = nullptr;
}

void UNSStageObjectiveWidget::UpdateTransitionCountdown()
{
	if (!IsValid(CachedRunGameState)
		|| !TransitionCountdownText)
	{
		return;
	}

	const int32 RemainingSeconds = FMath::Max(
		0,
		FMath::CeilToInt(
			CachedRunGameState->GetBossGateTimeRemaining()));

	TransitionCountdownText->SetText(
		FText::AsNumber(RemainingSeconds));
}

void UNSStageObjectiveWidget::RefreshStageObjective()
{
	if (!IsValid(CachedRunGameState)
		|| !ObjectiveOverlay
		|| !ObjectiveMessageText
		|| !ObjectiveProgressText)
	{
		return;
	}

	if (CachedRunGameState->StagePhase
		== ENSStagePhase::BossFight)
	{
		ObjectiveOverlay->SetVisibility(
			ESlateVisibility::Collapsed);

		RefreshBossGate();
		return;
	}

	ObjectiveOverlay->SetVisibility(
		ESlateVisibility::HitTestInvisible);

	const FNSStageObjectiveState& State =
		CachedRunGameState->ObjectiveState;

	FText ObjectiveMessage = State.Description;

	if (ObjectiveMessage.IsEmpty())
	{
		switch (State.Type)
		{
		case ENSStageObjectiveType::KillCount:
			ObjectiveMessage =
				LOCTEXT("KillCountObjective", "섬멸");
			break;

		case ENSStageObjectiveType::RescueNPC:
			ObjectiveMessage =
				LOCTEXT("RescueNPCObjective", "NPC 구출");
			break;

		default:
			ObjectiveMessage =
				LOCTEXT("UnknownObjective", "목표");
			break;
		}
	}

	ObjectiveMessageText->SetText(ObjectiveMessage);

	switch (State.Type)
	{
	case ENSStageObjectiveType::KillCount:
		{
			FFormatNamedArguments FormatArguments;
			FormatArguments.Add(
				TEXT("Current"),
				FText::AsNumber(State.Current));
			FormatArguments.Add(
				TEXT("Target"),
				FText::AsNumber(State.Target));

			ObjectiveProgressText->SetText(
				FText::Format(
					LOCTEXT(
						"KillProgressFormat",
						"{Current} / {Target}"),
					FormatArguments));
			break;
		}

	case ENSStageObjectiveType::RescueNPC:
		ObjectiveProgressText->SetText(
			State.Current >= State.Target
				? LOCTEXT("Rescued", "구출 완료")
				: LOCTEXT("NotRescued", "미구출"));
		break;

	default:
		ObjectiveProgressText->SetText(FText::GetEmpty());
		break;
	}

	RefreshBossGate();
}

void UNSStageObjectiveWidget::RefreshBossGate()
{
	if (!IsValid(CachedRunGameState)
		|| !TransitionOverlay
		|| !TransitionMessageText
		|| !TransitionCountdownText)
	{
		return;
	}

	if (CachedRunGameState->StagePhase
		!= ENSStagePhase::BossReady)
	{
		TransitionOverlay->SetVisibility(
			ESlateVisibility::Collapsed);
		return;
	}

	TransitionOverlay->SetVisibility(
		ESlateVisibility::HitTestInvisible);

	const float RemainingTime =
		CachedRunGameState->GetBossGateTimeRemaining();

	if (RemainingTime <= 0.0f)
	{
		TransitionMessageText->SetText(
			LOCTEXT(
				"ObjectiveCompleteMessage",
				"목표 달성\n보스룸으로 이동하세요"));

		TransitionCountdownText->SetVisibility(
			ESlateVisibility::Collapsed);
		return;
	}

	if (CachedRunGameState->bBossGateAllPresent)
	{
		TransitionMessageText->SetText(
			LOCTEXT(
				"BossStartMessage",
				"보스전이 시작됩니다"));
	}
	else
	{
		TransitionMessageText->SetText(
			LOCTEXT(
				"BossTeleportMessage",
				"보스 룸으로 순간이동 됩니다"));
	}

	TransitionCountdownText->SetVisibility(
		ESlateVisibility::HitTestInvisible);

	UpdateTransitionCountdown();
}

#undef LOCTEXT_NAMESPACE
