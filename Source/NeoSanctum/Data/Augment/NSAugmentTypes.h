// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NSAugmentTypes.generated.h"

class UNSAugmentDefinition;

UENUM(BlueprintType)
enum class ENSAugmentRarity : uint8
{
	Common    UMETA(DisplayName = "Common"),
	Rare      UMETA(DisplayName = "Rare"),
	Epic      UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary"),
};

/**
 * 증강 하나의 CombatStat Modifier를 정의하는 DataTable Row입니다.
 *
 * 같은 AugmentTag를 가진 Row들은 하나의 증강으로 그룹핑됩니다.
 * 그룹 내 메타 정보는 모두 동일해야 하며, 데이터 검증 단계에서 확인합니다.
 */
USTRUCT(BlueprintType)
struct FNSAugmentDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()
	
	// 증강 하나를 식별하는 고유 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Identity",
		meta = (Categories = "Augment.Definition"))
	FGameplayTag AugmentTag;
	
	// 증강이 속한 캐릭터 범위. Character.Player.Ranger 또는 Character.Common 등을 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Identity",
		meta = (Categories = "Character"))
	FGameplayTag OwnerCharacterTag;
	
	// Add 또는 Multiply 연산 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Modifier")
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;
	
	// Multiply는 스택당 퍼센트로 입력합니다. 10은 스택당 +10%, -10은 스택당 -10%.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Modifier",
		meta = (ToolTip = "Multiply는 스택당 퍼센트입니다. 10 = +10%, -10 = -10%."))
	float ValuePerStack = 0.0f;
	
	// 같은 등급 후보군 안에서의 증강 선택 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Meta",
		meta = (ClampMin = "1"))
	int32 SelectionWeight = 100;
	
	// 선택 가능한 최대 누적 스택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Meta",
		meta = (ClampMin = "1"))
	int32 MaxStack = 5;
	
	// 같은 AugmentTag Row들은 동일한 희귀도를 가져야 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Meta")
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;
	
	// Modifier가 적용될 대상 Ability 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Modifier",
		meta = (Categories = "Ability"))
	FGameplayTag TargetAbilityTag;
	
	// Modifier가 변경할 CombatStat 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Modifier",
		meta = (Categories = "CombatStat"))
	FGameplayTag StatTag;

	/**
	 * 카드 UI와 특수 GA / GE 설정을 제공하는 증강 Definition.
	 * 같은 AugmentTag를 가진 Row는 같은 Definition을 사용해야 합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Definition")
	TSoftObjectPtr<UNSAugmentDefinition> Definition;
	
	// false면 후보와 Modifier 캐시에서 제외
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Meta")
	bool bEnabled = true;
	
	/**
	 * true면 이 증강을 처음 획득할 때 Legendary 슬롯 하나를 점유.
	 * Rarity와 분리해 Legendary 수치 증강과 Legendary 기믹 증강을 구분.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Meta")
	bool bCountAsLegendarySlot = false;
};

// 인런 한정 보유 인스턴스, 핸들은 서버에서만 유효
USTRUCT(BlueprintType)
struct FNSAugmentInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId DefId;

	// 빠른 판정용 캐싱(레플리케이션 포함), UI에서 등급별 색상/아이콘 분기 시에도 사용
	UPROPERTY(BlueprintReadOnly)
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;

	UPROPERTY(BlueprintReadOnly)
	int32 Stacks = 0;
	
	// 레전더리 슬롯을 차지하는지 true면 기믹, false면 수치강화 -> 리플레케이션을 위해서 bool변수로
	UPROPERTY(BlueprintReadOnly)
	bool bCountsAsLegendarySlot = false;

	// 서버 권한 전용, 클라이언트에서는 항상 무효
	FActiveGameplayEffectHandle EffectHandle;
	FGameplayAbilitySpecHandle  AbilityHandle;
};

/**
 * 서버가 확정한 증강 선택 카드 스냅샷.
 *
 * DefId는 실제 증강 적용 대상을 식별하고, Rarity는 카드별 UI 표현을 위해 함께 전달한다.
 *
 * 클라이언트는 카드 Index만 서버에 요청하며, 서버는 PendingOffer[Index].DefId를 사용해 증강을 적용.
 */
USTRUCT(BlueprintType)
struct FNSAugmentSelectionCard
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Augment")
	FPrimaryAssetId DefId;
	
	UPROPERTY(BlueprintReadOnly, Category = "NS|Augment")
	ENSAugmentRarity Rarity = ENSAugmentRarity::Common;
};

namespace NSAugment
{
	// PercentPerStack / Stacks를 GE나 최종 계산에 사용할 배율 값으로 변환.
	FORCEINLINE float CalculateStackedMultiplyPercent(const float PercentPerStack, const int32 Stacks)
	{
		return 1.0f + (PercentPerStack * 0.01f * static_cast<float>(Stacks));
	}
}
