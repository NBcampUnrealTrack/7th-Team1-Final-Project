// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NSAugmentTypes.generated.h"

UENUM(BlueprintType)
enum class ENSAugmentRarity : uint8
{
	Common    UMETA(DisplayName = "Common"),
	Rare      UMETA(DisplayName = "Rare"),
	Epic      UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary"),
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
