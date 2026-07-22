// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSHitReactionData.generated.h"

// 월드 피격 리액션 조건을 GameplayCue로 매핑하는 DataTable Row
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitReactionData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	ENSHitFeedbackTargetType TargetType = ENSHitFeedbackTargetType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	ENSHitReactionDamageLayer DamageLayer = ENSHitReactionDamageLayer::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	ENSHitReactionAttackType AttackType = ENSHitReactionAttackType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	ENSHitFeedbackQuality HitQuality = ENSHitFeedbackQuality::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	ENSHitFeedbackOutcome Outcome = ENSHitFeedbackOutcome::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	FGameplayTag GameplayCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction")
	int32 Priority = 0;
};
