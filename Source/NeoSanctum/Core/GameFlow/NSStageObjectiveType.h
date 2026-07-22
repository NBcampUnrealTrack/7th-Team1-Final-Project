// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSStageObjectiveType.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ENSStageObjectiveType : uint8
{
	KillCount   UMETA(DisplayName = "Kill Count"),
	RescueNPC   UMETA(DisplayName = "Rescue NPC")
};

// 조건 풀에 넣을 목표 1개의 정의 (LevelConfig에서 편집)
USTRUCT(BlueprintType)
struct FNSStageObjective
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	ENSStageObjectiveType Type = ENSStageObjectiveType::KillCount;

	// KillCount: 필요한 처치 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective",
		meta = (ClampMin = "1",
				EditCondition = "Type == ENSStageObjectiveType::KillCount",
				EditConditionHides))
	int32 TargetKillCount = 30;

	// RescueNPC: 구출 대상 NPCId (비우면 아무 RescueNPC 구출 시 완료)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective",
		meta = (EditCondition = "Type == ENSStageObjectiveType::RescueNPC",
				EditConditionHides))
	FName TargetNPCId;

	// UI 표시용 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	FText Description;
};