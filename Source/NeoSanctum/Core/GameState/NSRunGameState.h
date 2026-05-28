// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSRunGameState.generated.h"

class ANSPlayerState;

/**
 *
 */
UCLASS()
class NEOSANCTUM_API ANSRunGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void GetAlivePlayerStates(TArray<ANSPlayerState*>& AlivePlayerStates, const ANSPlayerState* ExcludedPlayerState = nullptr) const;
};
