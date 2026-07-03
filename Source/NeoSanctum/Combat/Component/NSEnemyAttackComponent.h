// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NSEnemyAttackComponent.generated.h"

/**
 * Enemy의 사용 가능한 공격 Row 선택과 공격 쿨다운을 관리하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyAttackComponent();

	// 공격 쿨다운 기록을 초기화하는 함수
	void ResetAttackState();

	// 공격이 실제로 사용됐을 때 AttackId별 마지막 사용 시간을 기록하는 함수
	void RecordAttackUsed(const FNSEnemyAttackRow& AttackRow);

	// 현재 조건에서 사용할 공격 Row를 선택하는 함수
	const FNSEnemyAttackRow* SelectAttack(
		const AActor* TargetActor,
		const AActor* AttackActor,
		float Distance,
		bool bHasDirectLineOfSight,
		bool bSelectWeightedAttack) const;

	// 특정 공격 Row가 현재 조건에서 사용 가능한지 확인하는 함수
	bool CanUseAttack(
		const FNSEnemyAttackRow& AttackRow,
		const AActor* TargetActor,
		const AActor* AttackActor,
		float Distance,
		bool bHasDirectLineOfSight) const;

private:
	const UNSEnemyData* GetEnemyData() const;
	float GetOwnerHealthRatio() const;
	bool IsValidLivingTarget(const AActor* Target) const;
	bool CanUseDestructibleCoverAttack(
		const FNSEnemyAttackRow& AttackRow,
		const AActor* TargetActor,
		const AActor* AttackActor,
		bool bHasDirectLineOfSight) const;
	
	// Boss Mode 기준으로 공격 Row가 현재 Mode에서 사용 가능한지 확인하는 함수
	bool IsAttackAllowedByMode(const FNSEnemyAttackRow& AttackRow) const;

	// Owner가 BossModeComponent를 가지고 있으면 현재 Boss ModeTag를 반환하는 함수
	FGameplayTag GetOwnerBossModeTag() const;

private:
	// AttackId별 마지막 사용 시간을 저장하는 Map
	TMap<FName, float> LastAttackTimeById;
};
