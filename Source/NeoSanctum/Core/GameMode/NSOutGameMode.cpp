// Copyright 2026 One Team. All rights reserved.


#include "NSOutGameMode.h"
#include "NeoSanctum/Core/GameState/NSOutGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"

ANSOutGameMode::ANSOutGameMode()
{
	bUseSeamlessTravel = true;
	
	GameStateClass = ANSOutGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
}

void ANSOutGameMode::RequestStartRun_Implementation()
{
	ANSOutGameState* NSOutGameState = GetGameState<ANSOutGameState>();
	if (NSOutGameState)
	{
		if (NSOutGameState->IsAllPlayersReady())
		{
			GetWorld()->ServerTravel("/Game/NeoSanctum/Map/L_CanyonPlay");
		}
	}
}

void ANSOutGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	// TODO: 플레이어 입장 시 SaveData에서 진행도 받아서 연동해야함
}
