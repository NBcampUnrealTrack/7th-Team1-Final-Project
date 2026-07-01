// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_VanguardBaseAttack.generated.h"

UENUM(BlueprintType)
enum class ENSVanguardBaseAttackMode : uint8
{
	// 공격 모드 미결정 상태
	None,
	// 지상 기본 3단 콤보
	GroundCombo,
	// 공중 기본공격 내려찍기
	AirSlam,
	// 대쉬 직후 기본공격 홀드로 진입하는 차지 상태
	DashCharge
};

/**
 * 플레이어 근접 기본공격 Ability 골격.
 * 이 단계에서는 상태별 분기와 대쉬 차지 입력 수명만 관리하고,
 * 실제 몽타주/히트 판정/데미지는 후속 단계에서 붙인다.
 */
UCLASS()
class NEOSANCTUM_API UGA_VanguardBaseAttack : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_VanguardBaseAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

private:
	// 현재 캐릭터 상태 기준 기본공격 파생 모드 선택
	ENSVanguardBaseAttackMode SelectAttackMode(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 지상 기본 콤보 시작. 실제 콤보 재생은 후속 단계에서 연결
	void StartGroundCombo();

	// 공중 내려찍기 공격 시작. 실제 몽타주/착지 판정은 후속 단계에서 연결
	void StartAirSlam();

	// 대쉬공격 차지 시작 및 입력 해제 대기
	void StartDashCharge();

	// 대쉬공격 차지 종료 및 차지 비율 계산
	void FinishDashCharge();

	// 아직 몽타주가 없는 즉시형 모드 종료
	void FinishInstantMode();

	// Vanguard 공격 중 상태태그 부여
	void AddVanguardStateTags();

	// Vanguard 공격 중 상태태그 제거
	void RemoveVanguardStateTags();

	// 대쉬 1회당 대쉬 차지 진입 1회 제한용 입력 창 소비
	void ConsumeDashAttackWindow();

	// 대쉬 차지 시작 후 경과 시간 반환
	float GetDashChargeElapsedTime() const;

private:
	// 대쉬공격 최대 충전 도달 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashCharge", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float MaxDashChargeTime = 0.8f;

	// 기본공격 모드 선택과 차지 결과 로그 출력 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogVanguardAttackMode = true;

	// 현재 활성화된 Vanguard 기본공격 파생 모드
	ENSVanguardBaseAttackMode ActiveAttackMode = ENSVanguardBaseAttackMode::None;

	// 차지 비율 계산용 대쉬 차지 시작 시각
	double DashChargeStartTime = 0.0;
};
