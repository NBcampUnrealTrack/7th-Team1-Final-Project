// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSPlayerAttackFeedbackData.generated.h"

// 플레이어 공격 결과를 크로스헤어 피드백 타입으로 매핑하는 DataTable Row
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSPlayerAttackFeedbackData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	ENSHitFeedbackTargetType TargetType = ENSHitFeedbackTargetType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	ENSHitFeedbackQuality HitQuality = ENSHitFeedbackQuality::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	ENSHitFeedbackOutcome Outcome = ENSHitFeedbackOutcome::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	ENSCrosshairAttackFeedbackType CrosshairFeedbackType = ENSCrosshairAttackFeedbackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	FName SoundID = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackFeedback")
	int32 Priority = 0;
};
