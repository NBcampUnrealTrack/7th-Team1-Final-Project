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
