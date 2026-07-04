// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NSCommonUpgradeTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ENSCommonUpgradeCategory : uint8
{
	Combat		UMETA(DisplayName = "전투"),
	Survival	UMETA(DisplayName = "생존"),
	Utility		UMETA(DisplayName = "유틸"),
};


/**
 * OutRun 계정 공통 영구 강화 노드 하나를 정의하는 Row.
 * RowName = NodeId (UNSPermanentSaveGame::CommonSkillLevels의 키와 동일).
 */
USTRUCT(BlueprintType)
struct FNSCommonUpgradeNodeRow : public FTableRowBase
{
	GENERATED_BODY()

	// UI 배치(전투/생존/유틸 컬럼) 기준.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Identity")
	ENSCommonUpgradeCategory Category = ENSCommonUpgradeCategory::Combat;

	// 이 노드가 강화하는 CombatStat (예: CombatStat.MaxHealth). Attribute 매핑 조회에 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Identity",
		meta = (Categories = "CombatStat"))
	FGameplayTag AttributeTag;

	// Add 또는 Multiply 연산 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Modifier")
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;

	// Add면 실수값, Multiply면 % (5 = +5%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Modifier",
		meta = (ToolTip = "Multiply는 레벨당 퍼센트입니다. 5 = 5%."))
	float ValuePerLevel = 0.0f;

	// 업그레이드 가능한 최대 레벨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Cost", meta = (ClampMin = "1"))
	int32 MaxLevel = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Cost", meta = (ClampMin = "0"))
	int64 BaseCost = 100;

	// 레벨당 더해지는 Cost (0이면 비활성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Cost", meta = (ClampMin = "0"))
	int64 CostGrowthFlat = 0;

	// 레벨당 곱해지는 증가율 % (0이면 비활성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Cost", meta = (ClampMin = "0"))
	float CostGrowthPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Display")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Display")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CommonUpgrade|Display", meta = (MultiLine = true))
	FText Description;
};
