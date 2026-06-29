// Copyright 2026 One Team. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "NSDifficultyType.generated.h"

/*
 *	인런 난이도 조절 조절용
 */

USTRUCT(BlueprintType)
struct FNSDifficultyScale
{
	GENERATED_BODY()
	
	// 더할 비율
	UPROPERTY(BlueprintReadOnly)
	float HealthAddRatio = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float DamageAddRatio = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float DefenseAddRatio = 0.0f;
	
	// (기본+가산)에 곱할 배율
	UPROPERTY(BlueprintReadOnly)
	float Multiply = 1.0f;
};
