// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSEnemyPawnBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UAbilitySystemComponent;
class UNSMonsterAttributeSet;
class UNSEnemyCombatComponent;
class UNSEnemyCoreComponent;
class UNSEnemyPhaseComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSEnemyStateComponent;
class UNSEnemyData;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.03
 *
 * 클래스 개요 : Character가 아닌 Pawn 기반 Enemy가 공유하는 공통 기반 클래스
 * ASC, AttributeSet, EnemyData 초기화, 공통 Enemy 컴포넌트, INSEnemyAgent 구현을 담당
*/
UCLASS(Abstract)
class NEOSANCTUM_API ANSEnemyPawnBase : public APawn,
                                        public IAbilitySystemInterface,
                                        public IGenericTeamAgentInterface,
                                        public INSEnemyAgent
{
	GENERATED_BODY()

public:
	ANSEnemyPawnBase();

	virtual void BeginPlay() override;

	// Enemy Pawn이 Enemy 팀으로 인식되도록 TeamId를 반환하는 함수
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// Enemy Pawn이 사용하는 EnemyData를 반환하는 함수
	virtual UNSEnemyData* GetEnemyData() const override;

	// Enemy Pawn의 ASC를 반환하는 함수
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	// Enemy Pawn의 주 Skeletal Mesh를 반환하는 함수
	virtual USkeletalMeshComponent* GetEnemyMesh() const override { return EnemyMesh; }

	// 현재 실행 중인 공격 Row를 저장하는 함수
	virtual void SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow) override;

	// 현재 실행 중인 공격 Row를 반환하는 함수
	virtual const FNSEnemyAttackRow* GetCurrentAttackRow() const override;

	// 현재 실행 중인 공격 Row를 초기화하는 함수
	virtual void ClearCurrentAttackRow() override;

	// Enemy Pawn이 피격 경직 상태인지 반환하는 함수
	virtual bool IsHitReacting() const override;

	// 공격, 조준, Trace 기준으로 사용할 위치를 반환하는 함수
	virtual FVector GetAimLocation() const override;

	// 외부에서 EnemyData를 주입하는 함수
	void SetEnemyData(UNSEnemyData* InEnemyData);

	// 난이도 배율을 CoreComponent에 전달하는 함수
	void SetDifficultyScale(const FNSDifficultyScale& InScale);

	// Enemy Pawn 사망 상태를 시작하는 함수
	void Die();

	// Enemy Pawn 생존 여부를 반환하는 함수
	bool IsDead() const;

	// 루트 충돌 컴포넌트를 반환하는 함수
	UCapsuleComponent* GetCollisionComponent() const { return CollisionComponent; }

	// EnemyData와 GAS 초기화 컴포넌트를 반환하는 함수
	UNSEnemyCoreComponent* GetCoreComponent() const { return CoreComponent; }

	// 현재 공격 Row 관리 컴포넌트를 반환하는 함수
	UNSEnemyCombatComponent* GetCombatComponent() const { return CombatComponent; }

	// Phase 관리 컴포넌트를 반환하는 함수
	UNSEnemyPhaseComponent* GetPhaseComponent() const { return PhaseComponent; }

	// 공격 선택 컴포넌트를 반환하는 함수
	UNSEnemyAttackComponent* GetAttackComponent() const { return AttackComponent; }

	// 공격 대상 Trace 컴포넌트를 반환하는 함수
	UNSEnemyTargetComponent* GetTargetComponent() const { return TargetComponent; }

	// Threat 관리 컴포넌트를 반환하는 함수
	UNSEnemyThreatComponent* GetThreatComponent() const { return ThreatComponent; }

	// 사망, 비활성, 피격 경직 상태 컴포넌트를 반환하는 함수
	UNSEnemyStateComponent* GetStateComponent() const { return StateComponent; }

protected:
	// EnemyData 기반으로 GAS와 외형 데이터를 초기화하는 함수
	virtual void InitializeFromData(bool bFullInit);

	// EnemyData가 바뀌었을 때 외형 데이터를 다시 적용하는 함수
	virtual void HandleEnemyDataChanged(UNSEnemyData* NewEnemyData);

	// EnemyData의 Mesh, AnimClass, Scale을 Pawn에 적용하는 함수
	virtual void ApplyVisualData();

	// 사망 시 충돌과 공격 상태를 정리하는 함수
	virtual void ApplyDeadState();

	// 살아있는 상태로 복구될 때 기본 상태를 복구하는 함수
	virtual void ApplyAliveState();

	// 사망 상태 변경 시 Alive/Dead 상태 적용 함수를 호출하는 함수
	virtual void HandleDeadStateChanged(bool bDead);

protected:
	// Enemy Pawn의 루트 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	// Enemy Pawn의 외형과 애니메이션을 담당하는 주 Skeletal Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> EnemyMesh;

	// Enemy Pawn의 현재 공격 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCombatComponent> CombatComponent;

	// Enemy Pawn의 EnemyData와 GAS 초기화를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCoreComponent> CoreComponent;

	// Enemy Pawn의 Phase 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyPhaseComponent> PhaseComponent;

	// Enemy Pawn의 공격 선택과 쿨다운을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyAttackComponent> AttackComponent;

	// Enemy Pawn의 공격 대상 판정과 Trace를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyTargetComponent> TargetComponent;

	// Enemy Pawn의 Threat 기록과 타깃 선택을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyThreatComponent> ThreatComponent;

	// Enemy Pawn의 사망, 비활성, 피격 경직 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyStateComponent> StateComponent;

	// Enemy Pawn의 Ability System Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	// Enemy Pawn의 체력, 피격 게이지 등을 보관하는 AttributeSet
	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;
};
