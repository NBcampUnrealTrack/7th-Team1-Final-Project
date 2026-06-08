// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSStageManager.generated.h"

DECLARE_DELEGATE(FNSOnStageCleared)

UCLASS()
class NEOSANCTUM_API UNSStageManager : public UObject
{
	GENERATED_BODY()

public:	
	
	// 맵 로딩 완료 시점에 GameMode에서 호출
	void SetEnemyCount(int32 Count);
	void AddEnemyCount(int32 Count);

	// 적 처치 시 GameMode에서 호출
	void HandleEnemyKilled();
	
	// 클리어 판정 완료 시 GameMode에 알릴 델리게이트
	FNSOnStageCleared OnStageCleared;

private:
	// 스테이지 클리어시 호출
	void CheckStageClearCondition();
	
	// 스테이지 남은 적 카운팅용
	int32 RemainingEnemyCount = 0;
	
};
