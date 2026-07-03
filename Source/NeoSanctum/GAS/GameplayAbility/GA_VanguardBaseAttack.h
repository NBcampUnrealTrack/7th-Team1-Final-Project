// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_VanguardBaseAttack.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

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
	DashCharge,
	// 대쉬 차지 입력 해제 후 발동되는 대쉬공격 상태
	DashAttack
};

/**
 * 플레이어 Vanguard 기본공격 Ability 골격.
 * 현재 단계에서는 상태별 분기, 대쉬 차지 입력 수명, 몽타주 재생만 관리함.
 * 실제 히트 판정/데미지/이동은 후속 단계에서 연결함.
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

	virtual void InputPressed(
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
	UFUNCTION()
	void OnAttackMontageCompleted();

	UFUNCTION()
	void OnAttackMontageInterrupted();

	UFUNCTION()
	void OnComboWindowOpened(FGameplayEventData Payload);

	UFUNCTION()
	void OnDashAttackRecoverStarted(FGameplayEventData Payload);

	UFUNCTION()
	void OnDashAttackMoveFinished();

	// 현재 캐릭터 상태 기준 기본공격 파생 모드 선택
	ENSVanguardBaseAttackMode SelectAttackMode(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 지상 기본 콤보 시작
	void StartGroundCombo();

	// 콤보 입력 처리
	void HandleGroundComboInput();

	// Combo Window Open 이벤트 대기
	void StartComboWindowEventTask();

	// 다음 콤보 섹션으로 이동
	bool TryAdvanceGroundCombo();

	// 공중 내려찍기 공격 시작
	void StartAirSlam();

	// 대쉬공격 차지 시작 및 입력 해제 대기
	void StartDashCharge();

	// 대쉬공격 차지 종료 및 차지 비율 계산
	void FinishDashCharge();

	// 대쉬 차지 해제 후 대쉬공격 몽타주 재생
	void StartDashAttack(float ChargeRatio);

	// 통합 몽타주 Attack Section 이동
	bool JumpToDashAttackSection();

	// 차지 비율 기반 대쉬공격 돌진 이동 시작
	bool StartDashAttackMovement(float ChargeRatio);

	// 대쉬공격 몽타주와 이동 완료 확인
	void TryEndDashAttack();

	// 대쉬공격 Recover 시작 이벤트 대기
	void StartDashAttackRecoverEventTask();

	// 대쉬공격 GameplayCue 시작
	void AddDashAttackGameplayCue();

	// 대쉬공격 GameplayCue 종료
	void RemoveDashAttackGameplayCue();

	// 화면 중앙 조준점 기준 대쉬공격 방향 계산
	bool TryGetDashAttackDirection(FVector& OutDirection) const;

	// 화면 중앙 조준점 추적
	bool TryGetCrosshairTarget(FVector& OutTarget) const;

	// 아직 몽타주가 없는 즉시형 모드 종료
	void FinishInstantMode();

	// 몽타주 완료/취소 시 Ability 종료
	bool PlayAttackMontageAndWait(UAnimMontage* Montage, float PlayRate);

	// Vanguard 공격 중 상태태그 부여
	void AddVanguardStateTags();

	// Vanguard 공격 중 상태태그 제거
	void RemoveVanguardStateTags();

	// 대쉬 1회당 대쉬 차지 진입 1회 제한용 입력 창 소비
	void ConsumeDashAttackWindow();

	// 대쉬 차지 시작 후 경과 시간 반환
	float GetDashChargeElapsedTime() const;

private:
	// 지상 기본 콤보 테스트용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> GroundComboMontage;

	// 공중 내려찍기 테스트용 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> AirSlamMontage;

	// 대쉬공격 차지 유지 중 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DashChargeMontage;

	// Vanguard 기본공격 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float AttackMontagePlayRate = 1.0f;

	// 지상 기본 콤보 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	TArray<FName> GroundComboSectionNames = { TEXT("Combo_1"), TEXT("Combo_2"), TEXT("Combo_3") };

	// 통합 대쉬공격 몽타주의 공격 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	FName DashAttackSectionName = TEXT("Attack");

	// 대쉬공격 최대 충전 도달 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashCharge", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float MaxDashChargeTime = 1.0f;

	// 대쉬공격 최소 돌진 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashAttack", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashAttackMinDistance = 350.f;

	// 대쉬공격 최대 돌진 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashAttack", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashAttackMaxDistance = 1000.f;

	// 대쉬공격 돌진 지속 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashAttack", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DashAttackDuration = 0.18f;

	// 대쉬공격 크로스헤어 추적 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashAttack", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashAttackAimTraceRange = 10000.f;

	// 대쉬공격 돌진 중 중력 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|DashAttack", meta = (AllowPrivateAccess = "true"))
	bool bEnableGravityDuringDashAttack = true;

	// 기본공격 모드 선택과 차지 결과 로그 출력 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogVanguardAttackMode = true;

	// 현재 활성화된 Vanguard 기본공격 파생 모드
	ENSVanguardBaseAttackMode ActiveAttackMode = ENSVanguardBaseAttackMode::None;

	// 차지 비율 계산용 대쉬 차지 시작 시각
	double DashChargeStartTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboWindowEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> DashAttackRecoverEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DashAttackMoveTask;

	// 현재 재생 중인 지상 콤보 단계
	int32 CurrentGroundComboIndex = INDEX_NONE;

	// Combo Window 이전 선입력 저장
	bool bComboInputBuffered = false;

	// 같은 Combo Window 안에서 중복 섹션 이동 방지
	bool bComboAdvancedInCurrentWindow = false;

	// 첫 공격 입력이 콤보 입력으로 재사용되는 것 방지
	bool bGroundComboInitialInputReleased = false;

	// 대쉬공격 이동 태스크 시작 여부
	bool bDashAttackMoveStarted = false;

	// 대쉬공격 이동 태스크 완료 여부
	bool bDashAttackMoveFinished = false;

	// 대쉬공격 몽타주 Section 이동 성공 여부
	bool bDashAttackMontageStarted = false;

	// 대쉬공격 몽타주 완료 여부
	bool bDashAttackMontageFinished = false;
};
