// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyStateComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UNSEnemyData;
class UNSMonsterAttributeSet;

DECLARE_MULTICAST_DELEGATE(FNSEnemyDeathStarted);
DECLARE_MULTICAST_DELEGATE_OneParam(FNSEnemyDeadStateChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FNSEnemyInactiveStateChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FNSEnemyHitReactionStateChanged, bool);

/**
 * Enemy의 사망, 비활성, 피격 경직 상태를 Character/Pawn 공통으로 관리하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 Enemy 사망 처리가 시작될 때 발생하는 이벤트
	FNSEnemyDeathStarted OnDeathStarted;

	// 사망 상태가 변경됐을 때 서버와 클라이언트에서 발생하는 이벤트
	FNSEnemyDeadStateChanged OnDeadStateChanged;

	// 비활성 상태가 변경됐을 때 서버와 클라이언트에서 발생하는 이벤트
	FNSEnemyInactiveStateChanged OnInactiveStateChanged;

	// 피격 경직 상태가 변경됐을 때 서버와 클라이언트에서 발생하는 이벤트
	FNSEnemyHitReactionStateChanged OnHitReactionStateChanged;

	// StateComponent가 사용할 몬스터 AttributeSet 참조를 초기화하는 함수
	void InitState(UNSMonsterAttributeSet* InMonsterAttributes);

	// Enemy를 사망 상태로 전환하고 DeathAbility를 실행하는 함수
	void Die();

	// 풀링 또는 강제 비활성 상태를 설정하는 함수
	void SetInactive(bool bNewInactive);

	// 풀에서 재사용할 때 사망, 비활성, 피격 상태를 초기화하는 함수
	void ResetForReuse();

	// 피격 게이지 최대치 도달 시 HitReactionAbility 실행을 시도하는 함수
	void StartHitReaction();

	// HitReactionAbility 종료 후 피격 경직 상태를 해제하는 함수
	void FinishHitReaction();

	// 피격 게이지를 0으로 초기화하는 함수
	void ResetHitGauge();

	// 현재 피격 게이지를 반환하는 함수
	float GetHitGauge() const;

	// 최대 피격 게이지를 반환하는 함수
	float GetMaxHitGauge() const;

	// 피격 게이지 누적이 가능한 상태인지 반환하는 함수
	bool CanReceiveHitGauge() const;

	// Enemy가 사망 상태인지 반환하는 함수
	bool IsDead() const { return bDead; }

	// Enemy가 풀링 등으로 비활성화된 상태인지 반환하는 함수
	bool IsInactive() const { return bInactive; }

	// Enemy가 피격 경직 상태인지 반환하는 함수
	bool IsHitReacting() const { return bHitReacting; }

private:
	UFUNCTION()
	void OnRep_bDead();

	UFUNCTION()
	void OnRep_bInactive();

	UFUNCTION()
	void OnRep_bHitReacting();

	// 피격 경직 상태를 변경하고 이벤트를 발생시키는 함수
	void SetHitReactionState(bool bNewHitReacting);

	// Owner가 Authority를 가지고 있는지 확인하는 함수
	bool IsOwnerAuthority() const;

	// Owner의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetOwnerASC() const;

	// Owner의 EnemyData를 반환하는 함수
	UNSEnemyData* GetOwnerEnemyData() const;

	// Owner가 실행 중인 공격 Row를 초기화하는 함수
	void ClearOwnerAttackRow() const;

private:
	// Enemy가 사망 상태인지 서버에서 관리하고 클라이언트에 복제하는 변수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bDead, Category = "Enemy|State",
		meta = (AllowPrivateAccess = "true"))
	bool bDead = false;

	// Enemy가 풀링 등으로 비활성화된 상태인지 서버에서 관리하고 클라이언트에 복제하는 변수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bInactive, Category = "Enemy|State",
		meta = (AllowPrivateAccess = "true"))
	bool bInactive = false;

	// Enemy가 피격 경직 행동을 수행 중인지 서버에서 관리하고 클라이언트에 복제하는 변수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bHitReacting, Category = "Enemy|State",
		meta = (AllowPrivateAccess = "true"))
	bool bHitReacting = false;

	// 피격 게이지 초기화와 조회에 사용할 몬스터 AttributeSet
	UPROPERTY(Transient)
	TObjectPtr<UNSMonsterAttributeSet> MonsterAttributes;
};
