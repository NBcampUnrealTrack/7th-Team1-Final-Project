// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameState.h"

#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "Net/UnrealNetwork.h"

void ANSRunGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSRunGameState, RunEndPhase);
	DOREPLIFETIME(ANSRunGameState, bIsClear);
	DOREPLIFETIME(ANSRunGameState, PhaseEndServerTime);
	DOREPLIFETIME(ANSRunGameState, WinningChoice);
	DOREPLIFETIME(ANSRunGameState, NextVotes);
	DOREPLIFETIME(ANSRunGameState, HubVotes);
	DOREPLIFETIME(ANSRunGameState, RunResultData);
	DOREPLIFETIME(ANSRunGameState, CurrentRunConfig);
	DOREPLIFETIME(ANSRunGameState, CurrentLevelConfig);
	DOREPLIFETIME(ANSRunGameState, StagePhase);
	DOREPLIFETIME(ANSRunGameState, ObjectiveState);
	DOREPLIFETIME(ANSRunGameState, BossGateEndServerTime);
	DOREPLIFETIME(ANSRunGameState, bBossGateAllPresent);
	DOREPLIFETIME(ANSRunGameState, bDifficultyTimerRunning);
	DOREPLIFETIME(ANSRunGameState, DifficultyBaseElapsedSeconds);
	DOREPLIFETIME(ANSRunGameState, DifficultyStartServerTime);
	DOREPLIFETIME(ANSRunGameState, DifficultyStageNumber);
}

void ANSRunGameState::SetRunDataConfig(
	TSoftObjectPtr<UNSRunConfig> InRunConfig,
	TSoftObjectPtr<UNSLevelConfig> InLevelConfig)
{
	if (!HasAuthority())
	{
		return;
	}
	
	CurrentRunConfig = InRunConfig;
	CurrentLevelConfig = InLevelConfig;
	
	// 서버가 자신은 OnRep이 호출되지 않으므로 직접 브로드캐스트.
	OnRep_RunDataConfig();
	
	ForceNetUpdate();
}

bool ANSRunGameState::HasRunDataConfig() const
{
	return !CurrentRunConfig.IsNull() && !CurrentLevelConfig.IsNull();
}

void ANSRunGameState::OnRep_RunDataConfig()
{
	OnRunDataConfigChanged.Broadcast();
}

ANSRunGameState::ANSRunGameState()
{
	ProjectileManagerComponent = CreateDefaultSubobject<UNSProjectileManagerComponent>(TEXT("ProjectileManagerComponent"));
}

void ANSRunGameState::GetAlivePlayerStates(TArray<ANSPlayerState*>& AlivePlayerStates, const ANSPlayerState* ExcludedPlayerState) const
{
	AlivePlayerStates.Reset();

	// PlayerState 배열을 순회해서 살아있는 Player를 찾음
	for (APlayerState* PlayerState : PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		if (!NSPlayerState || NSPlayerState == ExcludedPlayerState || NSPlayerState->IsDead())
		{
			continue;
		}

		AlivePlayerStates.Add(NSPlayerState);
	}
}

void ANSRunGameState::OnRep_RunEndVotes()
{
	OnRunEndPhaseChanged.Broadcast();
	NotifyRunVoteChanged();
}

void ANSRunGameState::OnRep_PhaseEndServerTime()
{
	OnRunEndPhaseChanged.Broadcast();
}

float ANSRunGameState::GetPhaseTimeRemaining() const
{
	return FMath::Max(0.f, PhaseEndServerTime - GetServerWorldTimeSeconds());
}

void ANSRunGameState::OnRep_RunEndPhase()
{
	OnRunEndPhaseChanged.Broadcast();
}

void ANSRunGameState::SetRunEndPhase(ENSRunEndPhase NewPhase)
{
	RunEndPhase = NewPhase;
	ForceNetUpdate();
	
	// 호스트는 OnRep 함수 실행안하므로 수동 실행
	if (HasAuthority())          
	{
		OnRep_RunEndPhase();
	}
}

void ANSRunGameState::NotifyRunVoteChanged()
{
	OnRunEndVoteChanged.Broadcast();
}

void ANSRunGameState::OnRep_BossGate()
{
	OnBossGateChanged.Broadcast();
}

float ANSRunGameState::GetBossGateTimeRemaining() const
{
	if (BossGateEndServerTime <= 0.0f)
	{
		return 0.0f;
	}
	
	return FMath::Max(
		0.0f,
		BossGateEndServerTime - GetServerWorldTimeSeconds());
}

void ANSRunGameState::SetBossGateState(float InEndServerTime, bool bInAllPresent)
{
	if (!HasAuthority())
	{
		return;
	}
	
	BossGateEndServerTime = InEndServerTime;
	bBossGateAllPresent   = bInAllPresent;
	ForceNetUpdate();

	// 호스트는 OnRep 안 불리므로 수동 실행
	OnRep_BossGate();
}

void ANSRunGameState::OnRep_ObjectiveState()
{
	OnStageObjectiveChanged.Broadcast();
}

void ANSRunGameState::SetObjectiveState(const FNSStageObjectiveState& NewState)
{
	if (!HasAuthority())
	{
		return;
	}
	
	ObjectiveState = NewState;
	ForceNetUpdate();
	
	// 호스트 수동 작동
	OnRep_ObjectiveState();   
}

void ANSRunGameState::OnRep_StagePhase()
{
	OnStagePhaseChanged.Broadcast();
}

void ANSRunGameState::SetStagePhase(ENSStagePhase NewPhase)
{
	StagePhase = NewPhase;
	ForceNetUpdate();

	// 호스트는 OnRep이 안 불리므로 수동 실행
	if (HasAuthority())
	{
		OnRep_StagePhase();
	}
}

void ANSRunGameState::OnRep_RunResultData()
{
	OnRunEndPhaseChanged.Broadcast();
}

void ANSRunGameState::SetRunResultData(const FNSRunResultData& NewRunResultData)
{
	if (!HasAuthority())
	{
		return;
	}
	RunResultData = NewRunResultData;
	
	ForceNetUpdate();
	OnRep_RunResultData();
}

void ANSRunGameState::AddRunResultKillCount(int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	RunResultData.KillCount += FMath::Max(Amount, 0);

	ForceNetUpdate();
	OnRep_RunResultData();
}

void ANSRunGameState::SetDifficultyTimerState(
	bool bInRunning,
	float InBaseElapsedSeconds,
	int32 InStageNumber)
{
	if (!HasAuthority())
	{
		return;
	}

	bDifficultyTimerRunning = bInRunning;
	DifficultyBaseElapsedSeconds = FMath::Max(0.0f, InBaseElapsedSeconds);
	DifficultyStageNumber = InStageNumber;

	if (bDifficultyTimerRunning)
	{
		DifficultyStartServerTime = GetServerWorldTimeSeconds();
	}
	else
	{
		DifficultyStartServerTime = 0.0f;
	}

	ForceNetUpdate();
	OnRep_DifficultyTimerState();
}

void ANSRunGameState::OnRep_DifficultyTimerState()
{
	OnDifficultyTimerStateChanged.Broadcast();
}

float ANSRunGameState::GetDifficultyElapsedSeconds() const
{
	if (!bDifficultyTimerRunning)
	{
		return DifficultyBaseElapsedSeconds;
	}

	return DifficultyBaseElapsedSeconds + FMath::Max(
		0.0f,
		GetServerWorldTimeSeconds() - DifficultyStartServerTime);
}

bool ANSRunGameState::ShouldShowDifficultyTimer() const
{
	return bDifficultyTimerRunning || GetDifficultyElapsedSeconds() > 0.0f;
}

int32 ANSRunGameState::GetDifficultyLevel(float Interval) const
{
	if (Interval <= 0.0f)
	{
		return 1;
	}

	return FMath::FloorToInt(GetDifficultyElapsedSeconds() / Interval) + 1;
}

float ANSRunGameState::GetDifficultyProgressPercent(float Interval) const
{
	if (Interval <= 0.0f)
	{
		return 0.0f;
	}

	const float CurrentStepElapsed =
		FMath::Fmod(GetDifficultyElapsedSeconds(), Interval);

	return FMath::Clamp(CurrentStepElapsed / Interval, 0.0f, 1.0f);
}
