// Copyright 2026 One Team. All rights reserved.


#include "NSStageManager.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"



void UNSStageManager::SetEnemyCount(int32 Count)
{
	RemainingEnemyCount = Count;
	UE_LOG(LogTemp, Log, TEXT("초기 적 수: %d"), RemainingEnemyCount);
}

void UNSStageManager::HandleEnemyKilled()
{
	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);
	UE_LOG(LogTemp, Log, TEXT("남은 적: %d"), RemainingEnemyCount);

	if (RemainingEnemyCount <= 0)
	{
		CheckStageClearCondition();
	}
}

void UNSStageManager::CheckStageClearCondition()
{
	OnStageCleared.ExecuteIfBound();
}



