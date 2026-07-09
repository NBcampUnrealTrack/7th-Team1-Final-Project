// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NSCharacterStatsMessageTypes.generated.h"
UENUM(BlueprintType)
enum class ENSCharacterStatDisplayType : uint8
{
	Number,
	Percent,
	Seconds
};

/**
 * 캐릭터 스탯 UI에 표시할 단일 스탯 항목
 */
USTRUCT(BlueprintType)
struct FNSCharacterStatViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag StatTag;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	float Value = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	ENSCharacterStatDisplayType DisplayType = ENSCharacterStatDisplayType::Number;
};

/**
 * 캐릭터 스탯 UI가 GMS로 수신하는 스냅샷 메시지
 */
USTRUCT(BlueprintType)
struct FNSCharacterStatsSnapshotMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FNSCharacterStatViewData> Stats;
};