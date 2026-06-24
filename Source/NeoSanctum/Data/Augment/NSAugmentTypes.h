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
	
	// Multiply는 1.0이 기준. 1.2는 20% 증가, 0.8은 20% 감소.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Augment|Modifier",
		meta = (ToolTip = "Multiply는 0보다 커야 합니다."))
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
