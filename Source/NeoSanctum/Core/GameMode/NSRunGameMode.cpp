// Copyright 2026 One Team. All rights reserved.


#include "NSRunGameMode.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"


ANSRunGameMode::ANSRunGameMode()
{
	GameStateClass = ANSRunGameState::StaticClass();
	PlayerControllerClass = ANSPlayerController::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();
	
	bUseSeamlessTravel = true;
}

void ANSRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// TODO: 후에 StageManager 구현 후 연결해야함
}

void ANSRunGameMode::StartNextStage()
{
}

void ANSRunGameMode::OnStageCleared()
{
}

void ANSRunGameMode::NotifyPlayerDied(AController* DeadPlayer)
{
}

void ANSRunGameMode::HandleRunOver(bool bIsClear)
{
	if (bIsClear)
	{
		// 클리어 시 로직 및 거점 귀환
	}
	else
	{
		// 전멸 시 런 종료 및 거점 귀환
	}
}
