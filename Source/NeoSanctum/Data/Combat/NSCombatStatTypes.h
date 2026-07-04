// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "NSCombatStatTypes.generated.h"

/**
 * Ability별 기본 CombatStat 값을 정의하는 DataTable Row
 * AbilityTag + StatTag 조합은 하나의 고유한 기본값으로 취급됩니다.
 */
USTRUCT(BlueprintType)
struct FNSAbilityBaseStatRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (Categories = "Ability", ToolTip = "이 스탯이 적용될 Ability 태그입니다. 예: Ability.Ranger.ProjectileShot"))
	FGameplayTag AbilityTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (Categories = "CombatStat", ToolTip = "조회할 CombatStat 태그입니다. 예: CombatStat.ExplosionRadius"))
	FGameplayTag StatTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "증강, 파츠, 영구 강화가 적용되기 전의 기본값입니다."))
	float BaseValue = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "이 스탯이 증강/파츠/영구 강화로 수정 가능한지 여부입니다."))
	bool bModifiable = true;
};

/**
 * CombatStat Modifier의 계산 방식을 정의합니다.
 */
UENUM(BlueprintType)
enum class ENSCombatStatModifierOperation : uint8
{
	Add UMETA(
		DisplayName = "Add",
		ToolTip = "기본값에 Value를 더합니다. 예: Base 400, Value 150 → Final 550"
	),
	
	Multiply UMETA(
		DisplayName = "Multiply", 
		ToolTip = "기본값에 Value를 곱합니다. 예: Base 100, Value 1.2 → Final 120"
	)
};

/**
 * 증강/파츠/영구 강화가 CombatStat을 어떻게 수정하는지 정의하는 DataTable Row
 * SourceDefId + TargetAbilityTag + StatTag 조합으로 어떤 보정이 적용될지 결정
 */
USTRUCT(BlueprintType)
struct FNSCombatStatModifierRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (AllowedTypes = "NSAugmentData", 
			ToolTip = "이 Modifier를 제공하는 증강 데이터입니다. 예: NSAugmentData:Augment_Test"))
	FPrimaryAssetId SourceDefId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "이 Modifier가 적용될 대상 Ability 태그입니다. 예: Ability.Ranger.ProjectileShot"))
	FGameplayTag TargetAbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "이 Modifier가 수정할 CombatStat 태그입니다. 예: CombatStat.ExplosionRadius"))
	FGameplayTag StatTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "Add는 더하기, Multiply는 곱하기로 계산합니다."))
	ENSCombatStatModifierOperation Operation = ENSCombatStatModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "Add는 더할 값을 입력합니다. Multiply는 배율을 입력합니다. 1.0은 변화 없음, 1.2는 20% 증가, 0.8은 20% 감소입니다. Multiply는 0보다 커야 합니다."))
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "이 Row의 사용 여부입니다. Row를 삭제하지 않고 임시로 비활성화할 때 사용합니다."))
	bool bEnabled = true;
};

/**
 * Ability CombatStat 값을 GameplayEffect SetByCaller 태그로 전달하기 위한 구조체
 * 
 * 캐릭터가 아닌 개체의 AttributeSet을 Init_GE로 설정하는 경우(예, Turret)의 GE 설정을 
 * CombatStat 값 기준으로 GE에서 SetByCaller를 통해 받아와서 초기화시키기 위해 제작
 */
USTRUCT(BlueprintType)
struct FNSSetByCallerFromCombatStat
{
	GENERATED_BODY()

	// 조회할 CombatStat 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "조회할 CombatStat 태그. 예: CombatStat.FireRate"))
	FGameplayTag CombatStatTag;

	// GE에 전달할 SetByCaller 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat",
		meta = (ToolTip = "계산된 값을 전달할 GameplayEffect SetByCaller 태그."))
	FGameplayTag SetByCallerTag;
};

/**
 * GameplayEffect Spec에 적용할 SetByCaller
 */
USTRUCT(BlueprintType)
struct FNSSetByCallerMagnitude
{
	GENERATED_BODY()

	// 적용할 SetByCaller 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag SetByCallerTag;

	// 전달할 수치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	float Magnitude = 0.0f;
};

/**
 * Ability CombatStat 값을 런타임 로직에 전달하기 위한 매핑
 */
USTRUCT(BlueprintType)
struct FNSRuntimeStatFromCombatStat
{
	GENERATED_BODY()

	// 조회할 CombatStat 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag CombatStatTag;
};

/**
 * 런타임 로직에 적용할 CombatStat 값
 */
USTRUCT(BlueprintType)
struct FNSCombatStatMagnitude
{
	GENERATED_BODY()

	// 런타임 값 식별 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	FGameplayTag CombatStatTag;

	// 런타임 수치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NS|CombatStat")
	float Magnitude = 0.0f;
};
