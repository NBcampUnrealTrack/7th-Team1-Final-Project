// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"
#include "GA_EnemyAttackLaser.generated.h"

class UNSEnemyPartComponent;
class ANSBossAIController;
class UNSEnemyCosmeticComponent;
class UNSEnemyCombatComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.06
 * 
 * 클래스 개요 : TitanWalker의 Center Laser 포신 TraceSocket을 기준으로 직선형 지속 레이저 판정을 처리하는 공격 Ability
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackLaser : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackLaser();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InitializeAttack() override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	struct FNSLaserBeam
	{
		// 레이저 판정 시작 위치
		FVector Start = FVector::ZeroVector;

		// 레이저 판정 끝 위치
		FVector End = FVector::ZeroVector;
	};

	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 공격 Row 기준 레이저 Beam 목록을 반환하는 함수
	void GetCurrentLaserBeams(TArray<FNSLaserBeam>& OutBeams) const;

	// WarnTime 이후 실제 레이저 데미지 루프를 시작하는 함수
	void StartLaser();

	// 레이저 데미지 Tick 타이머를 시작하는 함수
	void BeginLaserDamage();

	// 레이저 데미지 Tick을 처리하는 함수
	void TickLaserDamage();

	// 레이저 지속 시간이 끝났을 때 Ability를 정상 종료하는 함수
	void CompleteLaser();

	// 레이저 관련 Timer를 정리하는 함수
	void ClearLaserTimers();

	// Beam 하나의 Sweep 범위 안에 들어온 타깃을 수집하는 함수
	void CollectTargetsForBeam(
		const FNSLaserBeam& Beam,
		TSet<TObjectKey<AActor>>& OutTargets) const;

	// TargetActor가 데미지를 받을 수 있는 유효한 대상인지 확인하는 함수
	bool IsValidDamageTarget(AActor* TargetActor) const;

	// TargetActor의 Bounds 기준 판정 위치를 반환하는 함수
	FVector GetTargetCheckLocation(AActor* TargetActor) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 Tick 데미지를 계산하는 함수
	float CalculateLaserDamage(const FNSEnemyAttackRow& AttackRow) const;

	// TargetActor에게 Laser DamageEffect를 적용하는 함수
	bool ApplyLaserDamageToTarget(AActor* TargetActor, const FNSEnemyAttackRow& AttackRow);

	// AttackRow 기준 레이저 사거리를 반환하는 함수
	float GetLaserRange(const FNSEnemyAttackRow& AttackRow) const;

	// AttackRow 기준 레이저 판정 반경을 반환하는 함수
	float GetLaserRadius(const FNSEnemyAttackRow& AttackRow) const;

	// DebugData 기준으로 레이저 직선 범위를 표시하는 함수
	void DrawDebugLaserBeam(
		const FNSLaserBeam& Beam,
		const FNSEnemyAttackRow& AttackRow) const;

private:
	// Laser Sweep에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Laser")
	TEnumAsByte<ECollisionChannel> LaserTraceChannel = NSCollisionChannels::EnemyWeaponTrace;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// Beam 발사 순간에 고정된 레이저 조준 위치 변수
	FVector LockedLaserAimPoint = FVector::ZeroVector;

	// 현재 레이저가 고정 조준 위치를 사용하는지 나타내는 변수
	bool bHasLockedLaserAimPoint = false;

	// WarnTime 대기 Timer
	FTimerHandle LaserStartTimerHandle;

	// Laser Tick 반복 Timer
	FTimerHandle LaserTickTimerHandle;

	// Laser 종료 Timer
	FTimerHandle LaserEndTimerHandle;

protected:
	// 현재 레이저 코스메틱 Start/Update/Stop을 묶는 InstanceId를 저장하는 변수
	int32 LaserCosmeticInstanceId = INDEX_NONE;

	// 레이저 코스메틱 위치 갱신 타이머를 저장하는 변수
	FTimerHandle LaserCosmeticUpdateTimerHandle;

	// 레이저 코스메틱 위치 갱신 간격을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cosmetic", meta = (ClampMin = "0.01"))
	float LaserCosmeticUpdateInterval = 0.03f;

	// 현재 Avatar의 EnemyCosmeticComponent를 반환하는 함수
	UNSEnemyCosmeticComponent* GetEnemyCosmeticComponent() const;

	// 레이저 차징 시작 코스메틱 이벤트를 전송하는 함수
	void SendLaserChargeStartCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const;

	// 레이저 차징 갱신 코스메틱 이벤트를 전송하는 함수
	void SendLaserChargeUpdateCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const;

	// 레이저 Beam 시작 코스메틱 이벤트를 전송하는 함수
	void SendLaserBeamStartCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const;

	// 레이저 Beam 갱신 코스메틱 이벤트를 전송하는 함수
	void SendLaserBeamUpdateCosmeticEvent(const TArray<FNSLaserBeam>& Beams) const;

	// 레이저 종료 코스메틱 이벤트를 전송하는 함수
	void SendLaserStopCosmeticEvent() const;

	// Beam 목록과 AttackRow 기준으로 Laser 코스메틱 이벤트 데이터를 구성하는 함수
	void BuildLaserCosmeticEvent(
		FNSCosmeticEventNetData& OutEventData,
		FGameplayTag EventTag,
		ENSCosmeticEventPhase Phase,
		const TArray<FNSLaserBeam>& Beams,
		float Duration) const;

	// WarnTime 동안 Charge 코스메틱 위치를 갱신하는 함수
	void TickLaserChargeCosmeticUpdate();

	// Beam 지속 시간 동안 Beam 코스메틱 위치를 갱신하는 함수
	void TickLaserBeamCosmeticUpdate();

private:
	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// 현재 공격 대상 Actor를 반환하는 함수
	AActor* ResolveAttackActor() const;

	// AimMode와 타깃 상태 기준으로 레이저 조준 위치를 계산하는 함수
	FVector ResolveLaserAimPoint(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// Muzzle 위치와 공격 대상 기준으로 레이저 진행 방향을 계산하는 함수
	FVector ResolveLaserDirection(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;


	// 적 CombatComponent를 가져오는 함수
	UNSEnemyCombatComponent* GetEnemyCombatComponent() const;

	// Beam 발사 순간의 조준 위치를 고정하는 함수
	bool LockLaserAimPoint();

	// Beam 종료 시 고정 조준 위치를 해제하는 함수
	void ClearLockedLaserAimPoint();

	// 고정된 조준 위치를 기준으로 레이저 방향을 계산하는 함수
	FVector ResolveLockedLaserDirection(const FNSEnemyAttackRow& AttackRow, const FTransform& MuzzleTransform) const;
};
