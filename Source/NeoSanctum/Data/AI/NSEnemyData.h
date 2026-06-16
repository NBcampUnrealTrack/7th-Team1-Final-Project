// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NSEnemyData.generated.h"

class ANSEnemyWeaponBase;
class USkeletalMesh;
class UAnimInstance;
class UDataTable;
class UGameplayEffect;
class UGameplayAbility;
class UBehaviorTree;
class UStateTree;
class UEnvQuery;

UENUM(BlueprintType)
enum class ENSEnemyMovementType : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	Flying UMETA(DisplayName = "Flying"),
	Stationary UMETA(DisplayName = "Stationary")
};

UENUM(BlueprintType)
enum class ENSEnemyCombatRole : uint8
{
	Melee UMETA(DisplayName = "Melee"),
	Ranged UMETA(DisplayName = "Ranged"),
	Hybrid UMETA(DisplayName = "Hybrid"),
	Summoner UMETA(DisplayName = "Summoner")
};

UENUM(BlueprintType)
enum class ENSEnemyRank : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss")
};

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

USTRUCT(BlueprintType)
struct FNSEnemyAttackCondition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float MaxRange = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	bool bRequireLineOfSight = true;
};

USTRUCT(BlueprintType)
struct FNSEnemyAttackDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	FName AttackId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	FNSEnemyAttackCondition Condition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	int32 Priority = 0;
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
	// 몬스터 식별 태그 (구조 개선 후 제거 예정)
	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FGameplayTag EnemyTag;
	
	// 몬스터 고유 식별자
	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FGameplayTag EnemyId;

	// 몬스터 설명
	UPROPERTY(EditDefaultsOnly, Category = "Identity", meta = (MultiLine = true))
	FString Description;
	
	// 이동 분류
	UPROPERTY(EditDefaultsOnly, Category = "Classification")
	ENSEnemyMovementType MovementType = ENSEnemyMovementType::Ground;

	// 전투 역할 분류
	UPROPERTY(EditDefaultsOnly, Category = "Classification")
	ENSEnemyCombatRole CombatRole = ENSEnemyCombatRole::Melee;

	// 몬스터 등급
	UPROPERTY(EditDefaultsOnly, Category = "Classification")
	ENSEnemyRank EnemyRank = ENSEnemyRank::Normal;

	// 스켈레탈 메시
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	// 몬스터 크기 배율
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FVector DrawScale;

	// 무기
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<ANSEnemyWeaponBase> DefaultWeaponClass;

	// 초기 스탯
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TObjectPtr<UDataTable> AttributeInitData;

	// 스폰 시 GE
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	// 기본 GA
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	// 몬스터가 사용할 수 있는 공격 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<FNSEnemyAttackDefinition> AttackList;

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
