// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Core/Interface/NSHitReactionSourceInterface.h"
#include "GA_VanguardBaseAttack.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class ANSMeleeWeapon;

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
class NEOSANCTUM_API UGA_VanguardBaseAttack : public UGA_SkillBase,
                                             public INSHitReactionSourceInterface
{
	GENERATED_BODY()

public:
	UGA_VanguardBaseAttack();

	virtual ENSHitReactionAttackType GetHitReactionAttackType() const override { return ENSHitReactionAttackType::Melee; }

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

	UFUNCTION()
	void OnAirSlamHoverFinished();

	UFUNCTION()
	void OnAirSlamDiveFinished();

	UFUNCTION()
	void OnMeleeHitEventReceived(FGameplayEventData Payload);

	// 현재 캐릭터 상태 기준 기본공격 파생 모드 선택
	ENSVanguardBaseAttackMode SelectAttackMode(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 지상 기본 콤보 시작
	void StartGroundCombo();

	// 콤보 입력 처리
	void HandleGroundComboInput();

	// Combo Window Open 이벤트 대기
	void StartComboWindowEventTask();

	// Trace 이벤트 대기
	void StartMeleeHitEventTask();

	// 근접 공격 명중 이벤트 처리
	void HandleMeleeHitEvent(const FGameplayEventData& Payload);

	// 현재 장착 중인 근접 무기 조회
	ANSMeleeWeapon* GetCurrentMeleeWeapon() const;

	// 근접 무기 소켓 위치 기반 Sweep 판정 수행
	void PerformMeleeSocketSweeps(ANSMeleeWeapon& MeleeWeapon);

	// 단일 소켓 궤적 구간 Sweep 수행
	void SweepMeleeTrace(const FVector& TraceStart, const FVector& TraceEnd);

	// 근접 공격 데미지 적용
	void ApplyDamageToActor(const FHitResult& HitResult);

	// CombatStat.Damage 기준 최종 데미지 조회
	bool TryGetFinalDamage(float& OutDamage) const;

	// 현재 공격 파생 모드에 맞는 데미지 배율 계산
	float GetCurrentAttackDamageMultiplier() const;

	// 데미지 SetByCaller 적용
	void ApplyDamageSetByCaller(FGameplayEffectSpecHandle& InSpecHandle, float InDamage) const;

	// 데미지 감지 가해자 지정
	void AssignDamageInstigator(FGameplayEffectSpecHandle& InSpecHandle);

	// 근접 공격 Sweep 디버그 표시
	void DrawMeleeTraceDebug(const FVector& TraceStart, const FVector& TraceEnd, bool bHit, const FHitResult& HitResult) const;

	// 다음 콤보 섹션으로 이동
	bool TryAdvanceGroundCombo();

	// 공중 내려찍기 공격 시작
	void StartAirSlam();

	// 공중 내려찍기 Hover Section 시작
	void StartAirSlamHover();

	// 공중 내려찍기 Dive Section 시작
	void StartAirSlamDive();

	// 공중 내려찍기 Impact Section 시작
	void StartAirSlamImpact();

	// 공중 내려찍기 몽타주 Section 이동
	bool JumpToAirSlamSection(FName SectionName) const;

	// 공중 내려찍기 착지 목표 위치 계산
	bool TryGetAirSlamTargetLocation(FVector& OutTargetLocation) const;

	// 공중 내려찍기 이동 모드 복구
	void RestoreAirSlamMovementMode() const;

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

	// 돌진 GameplayCue 시작 : 대쉬 공격 돌진 상황 + 공중 공격 루프 상황에서 재활용 예정
	void AddAttackFlashGameplayCue();

	//  돌진 GameplayCue 종료 : 대쉬 공격 돌진 상황 + 공중 공격 루프 상황에서 재활용 예정
	void RemoveAttackFlashGameplayCue();

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

	// Vanguard 기본공격 AbilityTag 기준으로 CombatStat 최종값을 조회하고, 없으면 기본값을 사용
	float GetVanguardFinalStatOrDefault(const FGameplayTag& StatTag, float DefaultValue) const;

	// DashCharge 최대 차징 시간을 CombatStat.ChargingTime 기준으로 조회
	float GetFinalDashChargeTime() const;

	// 지상 콤보 공격속도를 CombatStat.AttackSpeed 기준으로 조회
	float GetFinalGroundComboPlayRate() const;

	// DashAttack 이동 수치를 CombatStat.MinSkillRange / SkillRange / Duration 기준으로 조회
	bool TryResolveDashAttackMovementStats(
		float& OutMinDistance,
		float& OutMaxDistance,
		float& OutDuration
	) const;

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

	// 지상 콤보 공격속도 최소 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Combo", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float MinGroundComboPlayRate = 0.5f;

	// 지상 콤보 공격속도 최대 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Combo", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float MaxGroundComboPlayRate = 1.5f;

	// 지상 기본 콤보 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	TArray<FName> GroundComboSectionNames = { TEXT("Combo_1"), TEXT("Combo_2"), TEXT("Combo_3") };

	// 통합 대쉬공격 몽타주의 공격 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	FName DashAttackSectionName = TEXT("Attack");

	// 공중 내려찍기 체공 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	FName AirSlamHoverSectionName = TEXT("Hover");

	// 공중 내려찍기 낙하 루프 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	FName AirSlamDiveSectionName = TEXT("Dive");

	// 공중 내려찍기 착지 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Animation", meta = (AllowPrivateAccess = "true"))
	FName AirSlamImpactSectionName = TEXT("Impact");

	// 공중 내려찍기 체공 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|AirSlam", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AirSlamHoverDuration = 0.25f;

	// 공중 내려찍기 낙하 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|AirSlam", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float AirSlamDiveSpeed = 3000.0f;

	// 공중 내려찍기 지면 탐색 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|AirSlam", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AirSlamGroundTraceDistance = 5000.0f;

	// 공중 내려찍기 착지 위치 보정 높이
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|AirSlam", meta = (AllowPrivateAccess = "true"))
	float AirSlamImpactGroundOffset = 5.0f;

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

	// 근접 공격 명중 시 적용할 데미지 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Melee", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 지상 콤보 단계별 데미지 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	TArray<float> GroundComboDamageMultipliers = { 1.0f, 1.1f, 1.25f };

	// 공중 내려찍기 데미지 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AirSlamDamageMultiplier = 1.2f;

	// 대쉬공격 최소 차징 데미지 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashAttackMinDamageMultiplier = 1.0f;

	// 대쉬공격 최대 차징 데미지 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashAttackMaxDamageMultiplier = 1.5f;

	// 근접 무기 소켓 궤적을 따라 Sweep할 구체 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Melee", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MeleeTraceRadius = 18.0f;

	// 근접 공격 Sweep 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawMeleeTraceDebug = false;

	// 근접 공격 Sweep 디버그 표시 지속 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MeleeTraceDebugDuration = 1.0f;

	// 기본공격 모드 선택과 차지 결과 로그 출력 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLogVanguardAttackMode = true;

	// 현재 활성화된 Vanguard 기본공격 파생 모드
	ENSVanguardBaseAttackMode ActiveAttackMode = ENSVanguardBaseAttackMode::None;

	// 차지 비율 계산용 대쉬 차지 시작 시각
	double DashChargeStartTime = 0.0;

	// 현재 대쉬공격 데미지 계산에 사용할 차지 비율
	float CurrentDashAttackChargeRatio = 0.0f;

	// 직전 근접 공격 판정 소켓 위치
	TArray<FVector> PreviousMeleeTraceSocketLocations;

	// 직전 근접 공격 판정 소켓 위치가 유효한지 여부
	bool bHasPreviousMeleeTraceSocketLocations = false;

	// 현재 근접 공격 판정 윈도우 식별자
	uint32 CurrentMeleeTraceWindowId = 0;

	// 현재 근접 공격 판정 윈도우에서 이미 데미지를 적용한 대상
	TSet<TObjectKey<AActor>> DamagedActorsInTraceWindow;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboWindowEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> MeleeHitEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> DashAttackRecoverEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DashAttackMoveTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> AirSlamHoverTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> AirSlamDiveTask;

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
