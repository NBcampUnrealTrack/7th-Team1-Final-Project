// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Core/GameFlow/NSDifficultyType.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSBossPawnBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UAbilitySystemComponent;
class UGameplayAbility;
class UNSMonsterAttributeSet;
class UNSEnemyCombatComponent;
class UNSEnemyCoreComponent;
class UNSEnemyPhaseComponent;
class UNSEnemyAttackComponent;
class UNSEnemyTargetComponent;
class UNSEnemyThreatComponent;
class UNSBossModeComponent;
class UNSBossTargetComponent;

UCLASS(Abstract)
class NEOSANCTUM_API ANSBossPawnBase : public APawn,
                                       public IAbilitySystemInterface,
                                       public IGenericTeamAgentInterface,
                                       public INSEnemyAgent
{
	GENERATED_BODY()

public:
	ANSBossPawnBase();

	virtual void BeginPlay() override;

	// Boss가 Enemy 팀으로 인식되도록 TeamId를 반환하는 함수
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// Boss가 사용하는 EnemyData를 반환하는 함수
	virtual UNSEnemyData* GetEnemyData() const override;

	// Boss의 ASC를 반환하는 함수
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	// Boss의 주 Skeletal Mesh를 반환하는 함수
	virtual USkeletalMeshComponent* GetEnemyMesh() const override { return BossMesh; }

	// 현재 실행 중인 공격 Row를 저장하는 함수
	virtual void SetCurrentAttackRow(const FNSEnemyAttackRow& InAttackRow) override;

	// 현재 실행 중인 공격 Row를 반환하는 함수
	virtual const FNSEnemyAttackRow* GetCurrentAttackRow() const override;

	// 현재 실행 중인 공격 Row를 초기화하는 함수
	virtual void ClearCurrentAttackRow() override;

	// Boss가 피격 경직 상태인지 반환하는 함수
	virtual bool IsHitReacting() const override
	{
		return StateComponent && StateComponent->IsHitReacting();
	}

	// 공격, 조준, Trace 기준으로 사용할 위치를 반환하는 함수
	virtual FVector GetAimLocation() const override;

	// 외부에서 Boss EnemyData를 주입하는 함수
	void SetEnemyData(UNSEnemyData* InEnemyData);

	// 난이도 배율을 CoreComponent에 전달하는 함수
	void SetDifficultyScale(const FNSDifficultyScale& InScale);

	// Boss 사망 상태를 시작하는 함수
	void Die();

	// Boss 생존 여부를 반환하는 함수
	bool IsDead() const
	{
		return StateComponent && StateComponent->IsDead();
	}
	
	// Boss의 ModeComponent를 반환하는 함수
	UNSBossModeComponent* GetBossModeComponent() const { return BossModeComponent; }

	// Boss의 다중 타깃 컴포넌트를 반환하는 함수
	UNSBossTargetComponent* GetBossTargetComponent() const { return BossTargetComponent; }
	
protected:
	// Boss의 루트 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	// Boss의 외형과 애니메이션을 담당하는 주 Skeletal Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> BossMesh;

	// Boss의 현재 공격 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCombatComponent> CombatComponent;

	// Boss의 EnemyData와 GAS 초기화를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyCoreComponent> CoreComponent;

	// Boss의 Phase 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyPhaseComponent> PhaseComponent;

	// Boss의 공격 선택과 쿨다운을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyAttackComponent> AttackComponent;

	// Boss의 공격 대상 판정과 Trace를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyTargetComponent> TargetComponent;

	// Boss의 Threat 기록과 타깃 선택을 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyThreatComponent> ThreatComponent;
	
	// Boss의 사망, 비활성, 피격 경직 상태를 관리하는 공통 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSEnemyStateComponent> StateComponent;
	
	// Boss의 현재 전투 Mode를 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossModeComponent> BossModeComponent;
	
	// Boss의 공격별 다중 타깃 목록을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNSBossTargetComponent> BossTargetComponent;

	// Boss의 Ability System Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	// Boss의 체력, 피격 게이지 등 보관하는 AttributeSet
	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> AttributeSet;

private:
	// EnemyData 기반으로 GAS와 외형 데이터를 초기화하는 함수
	void InitializeFromData(bool bFullInit);

	// EnemyData가 바뀌었을 때 외형 데이터를 다시 적용하는 함수
	void HandleEnemyDataChanged(UNSEnemyData* NewEnemyData);

	// EnemyData의 Mesh, AnimClass, Scale을 Boss에 적용하는 함수
	void ApplyVisualData();

	// Boss 사망 시 충돌과 공격 상태를 정리하는 함수
	void ApplyDeadState();

	// Boss가 다시 살아있는 상태가 될 때 기본 상태를 복구하는 함수
	void ApplyAliveState();
	
	// 사망 상태 반영 함수
	void HandleDeadStateChanged(bool bDead);
};
