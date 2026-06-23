// Copyright 2026 One Team. All rights reserved.


#include "NSStageManager.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"



void UNSStageManager::SetEnemyCount(int32 Count)
{
	RemainingEnemyCount = Count;
	UE_LOG(LogTemp, Log, TEXT("초기 적 수: %d"), RemainingEnemyCount);
}

void UNSStageManager::AddEnemyCount(int32 Count)
{
	RemainingEnemyCount += Count;
	UE_LOG(LogTemp, Log, TEXT("적 수 추가: %d, 총: %d"), Count, RemainingEnemyCount);
}

void UNSStageManager::HandleEnemyKilled()
{
	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);
	UE_LOG(LogTemp, Log, TEXT("남은 적: %d"), RemainingEnemyCount);
	
	// [임시] 누적 처치 수로 클리어 판정 (보스 구현 후 아래 원래 로직으로 복귀)
	++CurrentKillCount;
	UE_LOG(LogTemp, Log, TEXT("처치 수: %d / %d"), CurrentKillCount, KillsToClear);
	if (CurrentKillCount >= KillsToClear)
	{
		CheckStageClearCondition();
	}

	//if (RemainingEnemyCount <= 0)
	//{
	//	CheckStageClearCondition();
	//}
}

void UNSStageManager::CheckStageClearCondition()
{
	OnStageCleared.ExecuteIfBound();
}



