// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSHitReactionTypes.generated.h"

// 월드 피격 리액션 재생에 필요한 공통 Context
USTRUCT(BlueprintType)
struct NEOSANCTUM_API FNSHitReactionContext
{
	GENERATED_BODY()

	// 피격 위치
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	FVector HitLocation = FVector::ZeroVector;

	// 피격 표면 방향
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	FVector HitNormal = FVector::UpVector;

	// 공격을 유발한 액터
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	// 피격된 액터
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 실제 Health에 적용된 데미지량 : 데미지 표시 위젯이 들어오게되면 표시용
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	float DamageAmount = 0.0f;

	// 히트 판정 타입
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	ENSHitFeedbackQuality HitQuality = ENSHitFeedbackQuality::Normal;

	// 히트 결과
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "HitReaction")
	ENSHitFeedbackOutcome Outcome = ENSHitFeedbackOutcome::None;
};
