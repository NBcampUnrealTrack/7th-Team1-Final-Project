// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSEnemyData.generated.h"

class USkeletalMesh;
class UAnimInstance;
class ANSWeaponBase;
class UDataTable;
class UGameplayEffect;
class UGameplayAbility;
class UBehaviorTree;
class UStateTree;
class UEnvQuery;

USTRUCT(BlueprintType)
struct FNSMonsterAttributeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Defense = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseDamage = 50.0f;
};

USTRUCT(BlueprintType)
struct FNSPhaseDefinition
{
	GENERATED_BODY()

	// 페이즈 전환 체력
	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	float HPThresholdPercentage = 0.7f;

	// 페이즈 전환 시 실행한 무적/광폭화 GA
	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	TSubclassOf<UGameplayAbility> PhaseTransitionAbility;
};

/**
 * Enemy 초기화 시 필요한 PrimaryDataAsset 입니다.
 */
UCLASS(BlueprintType)
class NEOSANCTUM_API UNSEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UNSEnemyData();

public:
	// 몬스터 식별 태그
	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FGameplayTag EnemyTag;

	// 몬스터 설명
	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FString Description;

	// 스켈레탈 메시
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	// 로코모션 애니메이션
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<UAnimInstance> BaseAnimClass;

	// 몬스터 크기 배율
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FVector DrawScale;

	// 무기
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<ANSWeaponBase> DefaultWeaponClass;

	// 초기 스탯
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TObjectPtr<UDataTable> AttributeInitData;

	// 스폰 시 GE
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	// 기본 GA
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	TObjectPtr<UStateTree> StateTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	TObjectPtr<UEnvQuery> EQSQuery;

	// 최소 공격 사거리
	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	float MinAttackRange;

	// 최대 공격 사거리
	UPROPERTY(EditDefaultsOnly, Category = "AI Config")
	float MaxAttackRange;

	// 보스 페이즈
	UPROPERTY(EditDefaultsOnly, Category = "PhaseSystem")
	TArray<FNSPhaseDefinition> PhaseDefinitions;
};
