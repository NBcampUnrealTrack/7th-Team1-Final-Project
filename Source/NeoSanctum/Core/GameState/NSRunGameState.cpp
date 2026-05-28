// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameState.h"

#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"

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
