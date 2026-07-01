// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSEnemyControllerBase.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "Perception/AIPerceptionTypes.h" // FAIStimulus
#include "NSEnemyAIController.generated.h"

class UGameplayAbility;
class UNSEnemyData;
class ANSEnemyCharacterBase;
class UNSEnemyAttackComponent;
struct FNSEnemyAttackRow;
struct FNSEnemyPhaseRow;

// 타임스탬프 데미지
struct FNSThreatDamageSample
{
	double Timestamp = 0.0;
	float Damage = 0.0f;
};

// 최근 일정 시간의 데미지를 기록해 위험도 측정
struct FNSTargetThreatRecord
{
	TWeakObjectPtr<AActor> TargetActor;

	TArray<FNSThreatDamageSample> DamageSamples;

	FVector LastKnownLocation = FVector::ZeroVector;

	double LastSeenTime = -1.0;
	double LastStimulusTime = -1.0;

	bool bCurrentlyVisible = false;
};

UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public ANSEnemyControllerBase
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();
	virtual void Tick(float DeltaTime) override;

	// AI Controller가 적을 필터링하기 위한 TeamId 조회
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Enemy));
	}

	// 타 진영을 보았을 때의 적대/우호 규칙 정의 수립
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

	// 타겟과의 실시간 거리 기준으로 현재 사용할 공격 GA를 선택
	const FNSEnemyAttackRow* GetAttackRowByDistance();

	// Blackboard에 저장된 현재 공격 대상을 반환
	AActor* GetCurrentTargetActor() const;

public:
	// 타겟과의 현재 거리/방향/시야 기준으로 사용 가능한 공격이 하나라도 있는지 확인
	bool CanUseAnyAttackByDistance();

	// 공격이 실제로 활성화된 뒤 쿨다운 시작 시간을 기록
	void RecordAttackUsed(const FNSEnemyAttackRow& AttackRow);

protected:
	// 빙의 시점에 에디터에서 할당된 BT 가동
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 시각 센서가 갱신될 때 블랙보드로 데이터를 전달할 콜백 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	// 대상이 체력 데이터를 갖고 있는지, 살아 있는 유효한 타겟인지 검증
	bool IsValidLivingTarget(const AActor* Target) const;

	const FNSEnemyAttackRow* FindAttackRowByDistance(bool bSelectWeightedAttack);

protected:
	/* 시야/청각 설정 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/* 타겟을 정면으로 바라봤다고 판정할 최대 각도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AttackFacingAngleDegrees = 2.0f;

private:
	/* 타겟 액터 키 이름 */
	FName TargetActorKey = TEXT("TargetActor");
	
	// 현재 Possess 중인 Enemy의 Attack Component를 반환하는 함수
	UNSEnemyAttackComponent* GetEnemyAttackComponent() const;

private:
	void UpdateRetreatState(ANSEnemyCharacterBase* Enemy, const AActor* TargetActor);

	float GetMinimumAttackRange(const UNSEnemyData* EnemyData) const;

	FName ShouldRetreatKey = TEXT("bShouldRetreat");
	FName RetreatLocationKey = TEXT("RetreatLocation");

	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float RetreatExitBuffer = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float RetreatStepDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float RetreatDestinationAcceptanceRadius = 75.0f;

#pragma region 몬스터 어그로 관리

public:
	/* 함수 */

	// 현재 타깃을 향한 공격이 시작됐음을 기록하고 추적 제한 시간을 갱신하는 함수
	void NotifyAttackStarted();

private:
	// 현재 Threat 정보를 평가해 타깃 선택·유지·전환·해제를 처리하는 함수
	void UpdateTargetSelection();

	// 시각·청각·피해 감지 결과를 해당 타깃의 Threat 기록에 반영하는 함수
	void UpdateThreatFromStimulus(AActor* Actor, const FAIStimulus& Stimulus);

	// 만료된 피해 기록, 무효 타깃, 재선택 제한 정보를 정리하는 함수
	void PruneThreatRecords(double CurrentTime);

	// 피해량 우선, 피해가 없으면 거리 우선으로 가장 적합한 타깃을 찾는 함수
	AActor* FindBestTarget(double CurrentTime) const;

	// 설정된 시간 안에 특정 타깃이 가한 누적 피해량을 계산하는 함수
	float GetRecentDamageThreat(const FNSTargetThreatRecord& Record, double CurrentTime) const;

	// 시야·최근 감지·피해 기록을 기준으로 해당 타깃을 계속 기억할지 판단하는 함수
	bool IsThreatRecordRelevant(const FNSTargetThreatRecord& Record, double CurrentTime) const;

	// 타깃 잠금시간과 Threat 차이를 확인해 현재 타깃을 교체할지 결정하는 함수
	bool ShouldSwitchTarget(AActor* CandidateTarget, double CurrentTime) const;

	// 새 전투 타깃을 지정하고 선택 시간과 Blackboard 상태를 초기화하는 함수
	void SetCurrentCombatTarget(AActor* NewTarget);

	// 현재 타깃과 관련 Blackboard 값을 제거하고 필요하면 재선택을 잠시 차단하는 함수
	void ClearCurrentCombatTarget(bool bBlockReacquisition);

	// Threat, 타깃, 타이머 등 Controller의 모든 타깃 관리 상태를 초기화하는 함수
	void ResetTargetingState();

	// 현재 타깃, 마지막 위치, 시야 여부를 Blackboard에 반영하는 함수
	void UpdateCurrentTargetBlackboard();

	// 현재 타깃이 파괴 가능한 엄폐물 뒤에 있으면 추적 포기 타이머를 멈출 수 있는지 확인하는 함수
	bool CanMaintainCoverAttackTarget(AActor* TargetActor) const;

protected:
	/* 설정 변수 */

	/* 타깃 선택 로직을 다시 평가하는 최소 간격 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.05"))
	float TargetEvaluationInterval = 0.25f;

	/* 시야에서 사라진 타깃을 계속 기억하는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0"))
	float SightMemoryDuration = 2.0f;

	/* 청각 등 비시각 감지 정보를 유지하는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0"))
	float StimulusMemoryDuration = 3.0f;

	/* 누적 피해 Threat 계산에 포함할 최근 시간 범위 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.1"))
	float DamageThreatWindow = 10.0f;

	/* 새 타깃 선택 직후 타깃 변경을 막는 최소 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0"))
	float InitialTargetLockDuration = 2.5f;

	/* 한 번 타깃을 변경한 뒤 다시 변경할 수 있을 때까지의 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0"))
	float TargetSwitchCooldown = 2.0f;

	/* 새 타깃의 피해 Threat가 현재 타깃보다 얼마나 높아야 전환할지 기준 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "1.0"))
	float DamageThreatSwitchRatio = 1.25f;

	/* 피해 기록이 없을 때 새 타깃이 얼마나 더 가까워야 전환할지 기준 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DistanceSwitchRatio = 0.7f;

	/* 공격하지 못한 상태로 타깃을 계속 추적할 수 있는 최대 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.1"))
	float MaxPursuitWithoutAttackDuration = 10.0f;

	/* 추적 제한으로 포기한 타깃을 다시 선택하지 않는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Targeting", meta = (ClampMin = "0.0"))
	float TargetReacquireCooldown = 2.0f;

private:
	/* 런타임 변수 */

	/* 감지한 타깃별 시야, 위치, 피해 기록을 보관 */
	TMap<TWeakObjectPtr<AActor>, FNSTargetThreatRecord> ThreatRecords;

	/* 어그로가 해제된 타깃을 다시 선택할 수 있는 시간을 기록 */
	TMap<TWeakObjectPtr<AActor>, double> ReacquireBlockedUntil;

	/* 현재 AI가 선택한 전투 타깃 */
	TWeakObjectPtr<AActor> CurrentCombatTarget;

	/* 현재 타깃이 선택된 시점 */
	double CurrentTargetSelectedTime = 0.0;

	/* 마지막으로 타깃을 변경한 시점 */
	double LastTargetSwitchTime = 0.0;

	/* 현재 타깃을 향해 공격을 시작했거나 전투 진행이 발생한 마지막 시점 */
	double LastCombatProgressTime = 0.0;

	/* 다음 타깃 평가를 실행할 시점 */
	double NextTargetEvaluationTime = 0.0;

	/* 현재 타깃을 대상으로 한 공격이 한 번이라도 시작됐는지 여부 */
	bool bAttackStartedOnCurrentTarget = false;

	/* 마지막으로 확인한 타깃 위치를 저장할 Blackboard 키 이름 */
	FName TargetLastKnownLocationKey = TEXT("TargetLastKnownLocation");

	/* 현재 타깃이 시야에 보이는지 저장할 Blackboard 키 이름 */
	FName HasTargetLineOfSightKey = TEXT("bHasTargetLineOfSight");

#pragma endregion
#pragma region 근접 공격 예약

public:
	/* 함수 */
	
	/* 현재 전투 타깃에 근접 공격 예약을 요청하고 요청 처리 가능 여부를 반환하는 함수 */
	bool RequestMeleeAttackReservation();

	/* 현재 몬스터가 타깃의 활성 근접 공격 예약을 보유하고 있는지 확인하는 함수 */
	bool HasMeleeAttackReservation() const;

	/* 예약이 필요 없거나 활성 예약을 보유하여 현재 타깃에게 접근할 수 있는지 확인하는 함수 */
	bool CanApproachMeleeTarget() const;

	/* 현재 타깃에 예약 컴포넌트가 있고 이 몬스터가 근접 예약 대상인지 확인하는 함수 */
	bool CurrentTargetRequiresMeleeReservation() const;

	/* 실제 공격 시작을 예약 컴포넌트에 알려 예약 단계를 공격 중으로 전환하는 함수 */
	void NotifyMeleeReservationAttackStarted();

	/* 현재 활성 예약과 대기 요청을 반환하고 선택적으로 재획득 쿨다운을 적용하는 함수 */
	void ReleaseMeleeAttackReservation(bool bStartReacquireCooldown = true);

private:
	/* 현재 타깃과 예약 상태를 확인하여 예약 요청 및 블랙보드 값을 갱신하는 함수 */
	void UpdateMeleeReservationState();

	/* 예약 중인 타깃에 대한 활성 예약 또는 대기 요청을 취소하고 상태를 초기화하는 함수 */
	void CancelMeleeReservationRequest(bool bStartReacquireCooldown);

	/* 근접 몬스터 판정 함수 */
	bool UsesMeleeAttackReservation() const;

	/* 현재 타깃으로부터 마지막으로 피해를 받은 시간을 어그로 기록에서 가져오는 함수 */
	double GetLatestDamageTimeFromCurrentTarget() const;

	/* 근접 예약 보유 및 접근 가능 상태를 블랙보드에 기록하는 함수 */
	void SetMeleeReservationBlackboard(
		bool bHasReservation,
		bool bCanApproach);

protected:
	/* 런타임 변수 */
	
	/* 현재 예약을 요청했거나 활성 예약을 보유한 타깃 */
	TWeakObjectPtr<AActor> MeleeReservationTarget;

	/* 근접 공격 예약 보유 상태를 저장하는 블랙보드 키 이름 */
	FName HasMeleeAttackReservationKey = TEXT("bHasMeleeAttackReservation");

	/* 현재 근접 타깃에게 접근할 수 있는지를 저장하는 블랙보드 키 이름 */
	FName CanApproachMeleeTargetKey = TEXT("bCanApproachMeleeTarget");

#pragma endregion
#pragma region 근접 EQS

private:
	// EnemyData의 EQS Query를 Blackboard에 등록하고 런타임 상태를 초기화하는 함수
	void InitializeMeleeEQSBlackboard(const UNSEnemyData* EnemyData);

	// 현재 타깃이 변경됐을 때 이전 결과를 제거하고 재탐색을 요청하는 함수
	void ResetMeleeEQSForCurrentTarget();

	// EnemyData의 근접 EQS Query를 저장하는 Blackboard 키 이름
	FName MeleeEQSQueryKey = TEXT("MeleeEQSQuery");

	// EQS가 선택한 접근 위치를 저장하는 Blackboard 키 이름
	FName MeleeApproachLocationKey = TEXT("MeleeApproachLocation");

	// 근접 EQS를 다시 실행해야 하는지 저장하는 Blackboard 키 이름
	FName MeleeEQSNeedsRefreshKey = TEXT("bMeleeEQSNeedsRefresh");

#pragma endregion
#pragma region 피격 경직

public:
	// 피격 경직이 시작되면 이동과 공격을 중단하고 근접 예약을 유지하는 함수
	void HandleHitReactionStarted();

	// 피격 경직이 끝나면 Blackboard와 근접 예약 상태를 갱신하는 함수
	void HandleHitReactionFinished();

private:
	// 피격 경직 여부를 저장하는 Blackboard 키 이름
	FName IsHitReactingKey = TEXT("bIsHitReacting");

#pragma endregion
#pragma region 추적 방향

	// 추적·공격·후퇴 상태에 따라 이동 방향 회전과 타깃 방향 회전을 전환되는 함수
	void UpdateFacingMode(
		ANSEnemyCharacterBase* Enemy,
		AActor* TargetActor);

	// 현재 거리가 몬스터 공격 중 하나의 사용 가능 거리 안에 있는지 확인하는 함수
	bool IsWithinPotentialAttackRange(
		const ANSEnemyCharacterBase* Enemy,
		AActor* TargetActor) const;

	// 이동 방향 회전 또는 타깃 방향 회전 설정을 CharacterMovement에 적용하는 함수
	void ApplyFacingMode(
		ANSEnemyCharacterBase* Enemy,
		AActor* TargetActor,
		bool bFaceTarget);
#pragma endregion
#pragma region 타깃 트레이스

public:
	// Blackboard에 저장된 현재 발사 대상(플레이어 또는 엄폐물)을 반환하는 함수
	AActor* GetCurrentAttackActor() const;

private:

	// 플레이어까지 Trace해서 실제로 쏠 Actor를 계산하는 함수
	AActor* ResolveAttackActor(AActor* TargetActor, bool& bOutHasDirectLineOfSight) const;

	// Actor Bounds 기준 조준 위치를 계산하는 함수
	FVector GetAttackAimLocation(const AActor* Actor) const;

	// 엄폐물 판정 Trace 시작 위치를 계산하는 함수
	FVector GetCoverAttackTraceStart() const;

	// Blackboard의 AttackActor 키를 갱신하는 함수
	void SetAttackActorBlackboard(AActor* AttackActor);

	// 실제 발사 대상을 저장할 Blackboard 키 이름
	FName AttackActorKey = TEXT("AttackActor");

protected:
	// 파괴 가능 엄폐물이 시야를 막으면 그 엄폐물을 원거리 공격 대상으로 삼을지 여부
	UPROPERTY(EditDefaultsOnly, Category = "AI|Cover Attack")
	bool bAttackDestructibleCover = true;

	// 엄폐물 Trace/조준 위치를 Bounds 중심에서 위로 보정하는 비율
	UPROPERTY(EditDefaultsOnly, Category = "AI|Cover Attack", meta = (ClampMin = "0.0"))
	float CoverAttackAimZOffsetRatio = 0.15f;

#pragma endregion
};
