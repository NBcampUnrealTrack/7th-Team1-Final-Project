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
