// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSPartTypes.generated.h"

class UNSPartDefinition;

UENUM(BlueprintType)
enum class ENSPartSlot : uint8
{
	Body        UMETA(DisplayName = "바디"),
	Arm         UMETA(DisplayName = "암"),
	Leg         UMETA(DisplayName = "레그"),
};

UENUM(BlueprintType)
enum class ENSPartRarity : uint8
{
	Common      UMETA(DisplayName = "커먼"),
	Rare        UMETA(DisplayName = "레어"),
	Epic        UMETA(DisplayName = "에픽"),
	Legendary   UMETA(DisplayName = "레전더리"),
};

USTRUCT(BlueprintType)
struct FNSPartValueRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Min = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Max = 0.f;
};

/** 파츠 런타임 상태 (레플리케이션·저장 대상). GE 핸들은 UNSPartEquipComponent가 관리. */
USTRUCT(BlueprintType)
struct FNSPartData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UNSPartDefinition> DefinitionPtr;

	UPROPERTY(BlueprintReadOnly)
	ENSPartRarity CurrentRarity = ENSPartRarity::Common;

	UPROPERTY(BlueprintReadOnly)
	float CurrentValue = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 RollCount = 0;

	bool IsValid() const { return !DefinitionPtr.IsNull(); }
};
