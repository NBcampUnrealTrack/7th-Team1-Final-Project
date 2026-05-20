// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSOutGameState.generated.h"


UCLASS()
class NEOSANCTUM_API ANSOutGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	bool IsAllPlayersReady() const;
};
