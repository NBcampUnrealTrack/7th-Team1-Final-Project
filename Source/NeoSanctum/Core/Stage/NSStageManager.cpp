// Copyright 2026 One Team. All rights reserved.


#include "NSStageManager.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"



ANSStageManager::ANSStageManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSStageManager::BeginPlay()
{
	Super::BeginPlay();
}

void ANSStageManager::CheckStageClearCondition()
{
	// TODO: 후에 게임 종료 조건 OR 승리 조건 넣어야함.
	
	AGameModeBase* CurrentGameMode = GetWorld()->GetAuthGameMode();
	
	if (CurrentGameMode && CurrentGameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_NotifyStageCleared(CurrentGameMode);
	}
}



