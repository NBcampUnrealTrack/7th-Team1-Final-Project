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

void UNSStageManager::InitializeObjective(const FNSStageObjective& InObjective)
{
	CurrentObjective     = InObjective;
	ObjectiveProgress    = 0;
	bObjectiveComplete   = false;
	bObjectiveInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("목표 초기화 Type=%d, Target=%d"),
		static_cast<int32>(CurrentObjective.Type), GetObjectiveTarget());
}

void UNSStageManager::NotifyNPCRescued(FName RescuedNPCId)
{
	if (!bObjectiveInitialized || bObjectiveComplete)
	{
		return;
	}
	
	if (CurrentObjective.Type != ENSStageObjectiveType::RescueNPC)
	{
		return;
	}

	// 지정 대상이 있으면 일치할 때만 인정
	if (!CurrentObjective.TargetNPCId.IsNone()
		&& CurrentObjective.TargetNPCId != RescuedNPCId)
	{
		return;
	}

	ObjectiveProgress = 1;
	CheckObjectiveComplete();
}

int32 UNSStageManager::GetObjectiveTarget() const
{
	switch (CurrentObjective.Type)
	{
	case ENSStageObjectiveType::KillCount: return CurrentObjective.TargetKillCount;
	case ENSStageObjectiveType::RescueNPC: return 1;
	default:                               return 0;
	}
}

void UNSStageManager::CheckObjectiveComplete()
{
	if (bObjectiveComplete)
	{
		return;
	}
	
	if (ObjectiveProgress >= GetObjectiveTarget())
	{
		bObjectiveComplete = true;
		OnObjectiveComplete.ExecuteIfBound();
	}
}

void UNSStageManager::HandleEnemyKilled()
{
	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);

	if (!bObjectiveInitialized || bObjectiveComplete)
	{
		return;
	}
	
	if (CurrentObjective.Type != ENSStageObjectiveType::KillCount)
	{
		return;
	}

	++ObjectiveProgress;
	UE_LOG(LogTemp, Log, TEXT("[Objective] 처치 %d / %d"),
		ObjectiveProgress, GetObjectiveTarget());
	
	CheckObjectiveComplete();
}



