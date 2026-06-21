// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSMeleeAttackReservationComponent.generated.h"

class ANSEnemyCharacterBase;

/* 근접 공격 예약 요청 결과를 나타내는 열거형 */
enum class ENSMeleeReservationRequestResult : uint8
{
	/* 즉시 활성 슬롯을 배정받은 상태 */
	Reserved,

	/* 활성 슬롯이 없어 대기열에 등록된 상태 */
	Queued,

	/* 유효하지 않은 요청으로 예약과 대기열 등록이 모두 거절된 상태 */
	Rejected
};

/* 활성 예약을 보유한 몬스터의 현재 진행 단계를 나타내는 열거형 */
enum class ENSMeleeReservationPhase : uint8
{
	/* 예약을 획득하고 타깃에게 접근 중인 상태 */
	Approaching,

	/* 공격 Ability 또는 공격 몽타주를 실행 중인 상태 */
	Attacking
};

/* 공격 대상별 근접 공격자 수와 예약 대기열을 관리하는 범용 Actor Component */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSMeleeAttackReservationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* 컴포넌트 Tick과 기본 예약 설정을 초기화하는 생성자 */
	UNSMeleeAttackReservationComponent();

	/* 만료된 예약을 정리하고 대기 중인 몬스터를 빈 슬롯으로 승격시키는 Tick 함수 */
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/* 몬스터의 예약 요청을 처리하고 예약·대기·거절 결과를 반환하는 함수 */
	ENSMeleeReservationRequestResult RequestReservation(ANSEnemyCharacterBase* Enemy, double LastDamagedTime);

	/* 특정 몬스터가 현재 활성 예약을 보유하고 있는지 확인하는 함수 */
	bool HasReservation(ANSEnemyCharacterBase* Enemy);

	/* 접근 중인 예약을 공격 중 상태로 변경하고 긴 공격용 만료시간을 적용하는 함수 */
	void MarkAttackStarted(ANSEnemyCharacterBase* Enemy);

	/* 몬스터의 활성 예약과 대기 요청을 제거하고 필요하면 재획득 쿨다운을 시작하는 함수 */
	void ReleaseReservation(ANSEnemyCharacterBase* Enemy, bool bStartReacquireCooldown);

	/* 해당 타깃에 동시에 접근할 수 있는 최대 근접 몬스터 수를 설정하는 함수 */
	void SetMaxConcurrentAttackers(int32 InMaxAttackers);

private:
	/* 현재 활성 슬롯을 점유하고 있는 몬스터의 예약 정보를 나타내는 구조체 */
	struct FActiveReservation
	{
		/* 활성 슬롯을 점유 중인 몬스터의 약한 참조 */
		TWeakObjectPtr<ANSEnemyCharacterBase> Enemy;

		/* 몬스터가 접근 중인지 공격 중인지 나타내는 현재 예약 단계 */
		ENSMeleeReservationPhase Phase = ENSMeleeReservationPhase::Approaching;

		/* 예약이 자동으로 만료되는 월드 시간 */
		double ExpirationTime = 0.0;
	};

	/* 활성 슬롯을 기다리는 몬스터의 요청 정보를 나타내는 구조체 */
	struct FQueuedRequest
	{
		/* 예약 대기 중인 몬스터의 약한 참조 */
		TWeakObjectPtr<ANSEnemyCharacterBase> Enemy;

		/* 몬스터가 처음 대기열에 등록된 월드 시간 */
		double RequestTime = 0.0;

		/* 해당 타깃으로부터 몬스터가 가장 최근에 피해를 받은 월드 시간 */
		double LastDamagedTime = -1.0;
	};

	/* 사망·풀 반환·시간 만료 등으로 유효하지 않은 예약과 요청을 제거하는 함수 */
	void CleanupInvalidEntries(double CurrentTime);

	/* 비어 있는 활성 슬롯에 우선순위가 가장 높은 대기 요청을 배정하는 함수 */
	void PromoteQueuedRequests(double CurrentTime);

	/* 몬스터가 생존 상태이며 풀에 반환되지 않았는지 확인하는 함수 */
	bool IsEnemyValid(const ANSEnemyCharacterBase* Enemy) const;

	/* 몬스터가 공격 종료 후 재예약 금지시간에 포함되는지 확인하는 함수 */
	bool IsReacquireBlocked(ANSEnemyCharacterBase* Enemy, double CurrentTime) const;

	/* 최근 피격 여부와 대기시간을 기준으로 가장 우선순위가 높은 요청 인덱스를 찾는 함수 */
	int32 FindBestQueuedRequestIndex(double CurrentTime) const;
	
	/* 몬스터가 다시 예약을 획득할 수 없는 시간을 설정하는 함수 */
	void StartReacquireCooldown(ANSEnemyCharacterBase* Enemy, double CurrentTime);

private:
	/* 해당 타깃에 동시에 접근하거나 공격할 수 있는 최대 근접 몬스터 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxConcurrentAttackers = 3;

	/* 예약 획득 후 공격을 시작하지 못했을 때 접근 예약이 유지되는 최대 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float ApproachReservationDuration = 6.0f;

	/* 공격 종료 콜백 누락에 대비해 공격 중 예약을 강제로 정리하는 최종 안전시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float AttackReservationSafetyTimeout = 30.0f;

	/* 해당 타깃에게 최근 피격당한 몬스터로 인정하는 시간 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float RecentDamagePriorityDuration = 2.0f;

	/* 공격 종료 후 동일 몬스터가 슬롯을 다시 요청할 수 없는 최소 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ReacquireCooldownMin = 1.0f;

	/* 공격 종료 후 동일 몬스터가 슬롯을 다시 요청할 수 없는 최대 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Reservation",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ReacquireCooldownMax = 2.0f;

	/* 현재 접근 또는 공격 권한을 보유한 몬스터들의 활성 예약 목록 */
	TArray<FActiveReservation> ActiveReservations;

	/* 활성 슬롯이 비기를 기다리는 몬스터들의 요청 목록 */
	TArray<FQueuedRequest> QueuedRequests;

	/* 몬스터별로 예약을 다시 획득할 수 있는 월드 시간을 저장하는 맵 */
	TMap<TWeakObjectPtr<ANSEnemyCharacterBase>, double> ReacquireBlockedUntil;
};
