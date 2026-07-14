// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerAudioFlowComponent.h"

#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Data/Config/NSLevelConfig.h"

namespace NSAudioFlow
{
	static const FName TitleBGM(TEXT("Title_Thema"));
	static const FName OutRunBGM(TEXT("OutRun_Thema"));
	static constexpr float BGMRetryInterval = 0.2f;
	static constexpr int32 MaxBGMRetryCount = 30;
}

UNSPlayerAudioFlowComponent::UNSPlayerAudioFlowComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerAudioFlowComponent::HandleTitleLevelReady()
{
	PlayBGM(NSAudioFlow::TitleBGM);
}

void UNSPlayerAudioFlowComponent::HandleOutRunLevelReady()
{
	PlayBGM(NSAudioFlow::OutRunBGM);
}

void UNSPlayerAudioFlowComponent::HandlePreClientTravel()
{
	StopBGM(1.0f);
}

void UNSPlayerAudioFlowComponent::HandleClientRunDataReady()
{
	PlayCurrentLevelStageBGM();
}

void UNSPlayerAudioFlowComponent::BindRunGameState(ANSRunGameState* RunGameState)
{
	if (!ShouldHandleAudio() || !IsValid(RunGameState))
	{
		return;
	}

	if (ANSRunGameState* ExistingRunGameState = CachedRunGameState.Get())
	{
		ExistingRunGameState->OnStagePhaseChanged.RemoveDynamic(
			this,
			&ThisClass::HandleStagePhaseChanged);
	}

	CachedRunGameState = RunGameState;

	RunGameState->OnStagePhaseChanged.RemoveDynamic(
		this,
		&ThisClass::HandleStagePhaseChanged);
	RunGameState->OnStagePhaseChanged.AddDynamic(
		this,
		&ThisClass::HandleStagePhaseChanged);
}

void UNSPlayerAudioFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPendingBGM();

	if (ANSRunGameState* RunGameState = CachedRunGameState.Get())
	{
		RunGameState->OnStagePhaseChanged.RemoveDynamic(
			this,
			&ThisClass::HandleStagePhaseChanged);
	}
	CachedRunGameState.Reset();

	Super::EndPlay(EndPlayReason);
}

void UNSPlayerAudioFlowComponent::HandleStagePhaseChanged()
{
	if (!ShouldHandleAudio())
	{
		return;
	}

	const ANSRunGameState* RunGameState = CachedRunGameState.Get();
	if (!IsValid(RunGameState))
	{
		return;
	}

	if (RunGameState->StagePhase == ENSStagePhase::BossFight)
	{
		const UNSLevelConfig* LevelConfig = nullptr;
		if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
		{
			LevelConfig = Data->GetCurrentRunLevelConfig();
		}

		if (!LevelConfig)
		{
			LevelConfig = RunGameState->CurrentLevelConfig.Get();
		}

		if (LevelConfig)
		{
			PlayBGM(LevelConfig->BossBGM);
		}
		return;
	}

	if (RunGameState->StagePhase == ENSStagePhase::Objective)
	{
		PlayCurrentLevelStageBGM();
	}
}

void UNSPlayerAudioFlowComponent::PlayBGM(FName SoundID, float FadeOut, float FadeIn)
{
	if (!ShouldHandleAudio() || SoundID.IsNone() || CurrentBGMID == SoundID)
	{
		return;
	}

	PendingBGMID = SoundID;
	PendingFadeOut = FadeOut;
	PendingFadeIn = FadeIn;
	PendingBGMRetryCount = 0;
	bPendingBGMStoppedCurrent = false;

	if (!TryPlayPendingBGM())
	{
		ScheduleBGMRetry();
	}
}

bool UNSPlayerAudioFlowComponent::TryPlayPendingBGM()
{
	if (!ShouldHandleAudio() || PendingBGMID.IsNone())
	{
		return true;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
	{
		if (!bPendingBGMStoppedCurrent)
		{
			SoundSubsystem->StopBGM(PendingFadeOut);
			bPendingBGMStoppedCurrent = true;
		}

		if (UAudioComponent* BGMComponent =
			SoundSubsystem->PlayBGM(PendingBGMID, PendingFadeIn))
		{
			CurrentBGMID = PendingBGMID;
			ClearPendingBGM();
			return true;
		}
	}

	return false;
}

void UNSPlayerAudioFlowComponent::ScheduleBGMRetry()
{
	UWorld* World = GetWorld();
	if (!World || PendingBGMID.IsNone())
	{
		return;
	}

	if (PendingBGMRetryCount >= NSAudioFlow::MaxBGMRetryCount)
	{
		ClearPendingBGM();
		return;
	}

	World->GetTimerManager().SetTimer(
		BGMRetryTimerHandle,
		this,
		&ThisClass::RetryPendingBGM,
		NSAudioFlow::BGMRetryInterval,
		false);
}

void UNSPlayerAudioFlowComponent::RetryPendingBGM()
{
	if (PendingBGMID.IsNone())
	{
		return;
	}

	++PendingBGMRetryCount;

	if (!TryPlayPendingBGM())
	{
		ScheduleBGMRetry();
	}
}

void UNSPlayerAudioFlowComponent::ClearPendingBGM()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BGMRetryTimerHandle);
	}

	PendingBGMID = NAME_None;
	PendingFadeOut = 1.0f;
	PendingFadeIn = 1.0f;
	PendingBGMRetryCount = 0;
	bPendingBGMStoppedCurrent = false;
}

void UNSPlayerAudioFlowComponent::StopBGM(float FadeOut)
{
	if (!ShouldHandleAudio())
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(this))
	{
		ClearPendingBGM();
		SoundSubsystem->StopBGM(FadeOut);
		CurrentBGMID = NAME_None;
	}
}

void UNSPlayerAudioFlowComponent::PlayCurrentLevelStageBGM()
{
	if (!ShouldHandleAudio())
	{
		return;
	}

	const UNSLevelConfig* LevelConfig = nullptr;
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		LevelConfig = Data->GetCurrentRunLevelConfig();
	}

	if (!LevelConfig)
	{
		if (const ANSRunGameState* RunGameState = CachedRunGameState.Get())
		{
			LevelConfig = RunGameState->CurrentLevelConfig.Get();
		}
	}

	if (LevelConfig)
	{
		PlayBGM(LevelConfig->StageBGM);
	}
}

bool UNSPlayerAudioFlowComponent::ShouldHandleAudio() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	return PlayerController && PlayerController->IsLocalController();
}
