// Copyright 2026 One Team. All rights reserved.


#include "NSOutGameState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"

bool ANSOutGameState::IsAllPlayersReady() const
{
	if (PlayerArray.Num() == 0)
	{
		return false;
	}

	for (TObjectPtr<APlayerState> ExistingPlayerState : PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(ExistingPlayerState);
		if (NSPlayerState)
		{
			if (!NSPlayerState->IsReady())
			{
				return false;
			}
		}
		// PlayerState 캐스팅 실패시 진입 차단
		else
		{
			return false;
		}
	}

	return true;
}

void ANSOutGameState::NotifyReadyStateChanged()
{
	OnReadyStateChanged.Broadcast();
}
