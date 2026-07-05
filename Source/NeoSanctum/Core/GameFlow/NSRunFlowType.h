// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSRunFlowType.generated.h"

/**
 * GameFlow에서 사용하기 위한 Enum
 * 런 종료시의 상태나 투표에 사용
 */
 
UENUM(BlueprintType)
enum class ENSRunChoice : uint8
{
	ReturnToHub,
	NextStage
};

UENUM(BlueprintType)
enum class ENSRunEndPhase : uint8
{
	None,
	Voting,
	Result
};

UENUM(BlueprintType)
enum class ENSStagePhase : uint8
{
	// 스테이지 목표 수행 중
	Objective	UMETA(DisplayName = "Objective"),
    // 목표 달성, 보스 진입 볼륨 활성
    BossReady   UMETA(DisplayName = "Boss Ready"),
    // 전원 텔레포트 완료, 보스 전투
    BossFight   UMETA(DisplayName = "Boss Fight")
};
