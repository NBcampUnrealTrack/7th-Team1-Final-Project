// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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

USTRUCT(BlueprintType)
struct FNSPartDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	// DA 참조 -> 세이브/런타임 키와 일치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	TSoftObjectPtr<UNSPartDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	ENSPartSlot PartSlot = ENSPartSlot::Body;

	// 레그 파츠는 false (인런 밸런스상 리롤 불가)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	bool bCanReroll = true;

	// 언락(구매) 영구재화 비용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part",
		meta = (ClampMin = "0"))
	int64 UnlockCost = 0;

	// 등급별 수치 범위 (장착/리롤/등급업 시 이 범위에서 결정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	TMap<ENSPartRarity, FNSPartValueRange> ValueRange;

	// false면 카탈로그에서 제외
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Part")
	bool bEnabled = true;
};

/**
 * 파츠 런타임 상태 (레플리케이션/저장 대상)
 * GE 핸들은 UNSPartEquipComponent가 관리
 */
USTRUCT(BlueprintType)
struct FNSPartData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UNSPartDefinition> DefinitionPtr;

	// 슬롯 식별용
	UPROPERTY(BlueprintReadWrite)
	ENSPartSlot Slot = ENSPartSlot::Body;

	UPROPERTY(BlueprintReadWrite)
	ENSPartRarity CurrentRarity = ENSPartRarity::Common;

	UPROPERTY(BlueprintReadWrite)
	float CurrentValue = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 RollCount = 0;

	bool IsValid() const { return !DefinitionPtr.IsNull(); }
};
