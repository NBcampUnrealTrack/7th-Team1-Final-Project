// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyMeleeComponent.generated.h"

class ANSEnemyCharacterBase;
class UNSEnemyData;
class UNSMeleeAttackReservationComponent;

// 근접 예약 결과와 접근 가능 상태를 Controller에 전달하는 구조체
struct FNSMeleeState
{
	bool bAccepted = false;
	bool bHasReservation = false;
	bool bCanApproach = false;
};

/**
 * 일반 Enemy의 근접 공격 예약 상태와 예약 요청 흐름을 관리하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyMeleeComponent();

	// 예약 대상과 예약 상태를 초기화하는 함수
	void ResetMeleeState();

	// 현재 타겟에 근접 공격 예약을 요청하는 함수
	FNSMeleeState RequestReservation(AActor* TargetActor, double LastDamagedTime);

	// 현재 활성 근접 예약을 보유하고 있는지 확인하는 함수
	bool HasReservation() const;

	// 현재 타겟에게 접근 가능한지 확인하는 함수
	bool CanApproachTarget(AActor* TargetActor) const;

	// 현재 타겟이 근접 예약 컴포넌트를 요구하는지 확인하는 함수
	bool TargetRequiresReservation(AActor* TargetActor) const;

	// 예약 상태를 공격 중으로 전환하는 함수
	void NotifyAttackStarted();

	// 피격 경직으로 공격이 끊겼을 때 예약 상태를 접근 중으로 되돌리는 함수
	void MarkAttackInterrupted();

	// 현재 활성 예약 또는 대기 요청을 해제하는 함수
	void ReleaseReservation(bool bStartReacquireCooldown);

	// 현재 타겟 기준으로 예약 상태를 갱신하고 Blackboard에 반영할 상태를 반환하는 함수
	FNSMeleeState UpdateState(AActor* CurrentTarget);

	// 이 Enemy가 근접 예약을 사용하는 공격 구성을 갖고 있는지 확인하는 함수
	bool UsesReservation() const;

private:
	// Owner를 일반 Enemy Character로 반환하는 함수
	ANSEnemyCharacterBase* GetOwnerEnemy() const;

	// Owner의 EnemyData를 반환하는 함수
	const UNSEnemyData* GetEnemyData() const;

	// 타겟의 근접 예약 컴포넌트를 반환하는 함수
	UNSMeleeAttackReservationComponent* GetReservationComponent(AActor* TargetActor) const;

private:
	// 현재 예약을 요청했거나 활성 예약을 보유한 타겟
	TWeakObjectPtr<AActor> ReservationTarget;
};
