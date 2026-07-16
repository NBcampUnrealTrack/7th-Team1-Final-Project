// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NSCharacterBaseStatTypes.generated.h"

/**
 * 캐릭터 하나의 초기 Attribute 값을 정의하는 DataTable Row.
 * 하나의 Row가 하나의 캐릭터를 담당하며, RowName은 CharacterTag와 동일해야 합니다.
 */
USTRUCT(BlueprintType)
struct FNSCharacterBaseStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// UNSCharacterData::CharacterTag와 동일해야 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character", meta = (Categories = "Character"))
	FGameplayTag CharacterTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float Defense = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MoveSpeed = 600.0f;

	// 치명타 확률(%) / 20 = 20%
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float CritChance = 20.0f;

	// 치명타 피해 배율(%) / 150 = 기본 데미지의 150%
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float CritDamage = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxShield = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float ShieldRechargeRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float ShieldRechargeCooldown = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxDashCount = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float DashRegenRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxAmmo = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxSkill1Count = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxSkill2Count = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxSkill3Count = 1.0f;

	// 기본 점프 가능 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|Character|Attribute")
	float MaxJumpCount = 1.0f;

	// StatTag(CombatStat.*)에 해당하는 필드 값을 반환
	float GetValueForTag(const FGameplayTag& StatTag) const;
};
