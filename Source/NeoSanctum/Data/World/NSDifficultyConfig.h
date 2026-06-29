// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NSDifficultyConfig.generated.h"

/**
 *  인런 난이도 조절 계산용
 */

UCLASS(BlueprintType)
class NEOSANCTUM_API UNSDifficultyConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	// 시간→배율 곡선 설정하면 아래 단계식 무시, 아무것도 안 넣으면 단계식 사용
	UPROPERTY(EditAnywhere, Category="Time")
	TObjectPtr<UCurveFloat> TimeMultiplierCurve = nullptr;
	
	// 증가 1단계에 걸리는 시간(초), 작을수록 빨리 강해짐
	UPROPERTY(EditAnywhere, Category="Time", meta=(ClampMin="1.0"))
	float TimeStepInterval = 60.0f;

	// 시간 단계당 증가율(0.10 = 단계마다 +10%p)
	UPROPERTY(EditAnywhere, Category="Time", meta=(ClampMin="0.0"))
	float TimeIncreasePerStep = 0.10f;

	// 스테이지 1개당 증가율(0.20 = 스테이지마다 +20%p)
	UPROPERTY(EditAnywhere, Category="Stage", meta=(ClampMin="0.0"))
	float StageIncreasePerStage = 0.20f;

	// 추가 인원 1명당 기본 스탯에 더할 비율(0.30 = 1명당 +30%)
	UPROPERTY(EditAnywhere, Category="Players", meta=(ClampMin="0.0"))
	float PlayerHealthAddRatioPerExtra = 0.30f;
	UPROPERTY(EditAnywhere, Category="Players", meta=(ClampMin="0.0"))
	float PlayerDamageAddRatioPerExtra = 0.30f;
	UPROPERTY(EditAnywhere, Category="Players", meta=(ClampMin="0.0"))
	float PlayerDefenseAddRatioPerExtra = 0.30f;

	// 추가 인원 1명당 기본 스폰수에 더할 비율(0.50 = 1명당 +50%)
	UPROPERTY(EditAnywhere, Category="Players", meta=(ClampMin="0.0"))
	float SpawnCountAddRatioPerExtra = 0.50f;

	// Multiply 상한, 0이면 무제한
	UPROPERTY(EditAnywhere, Category="Limit", meta=(ClampMin="0.0"))
	float MaxMultiply = 0.0f;

	FNSDifficultyScale Evaluate(float ElapsedSeconds, int32 StageNumber, int32 PlayerCount) const;
};
