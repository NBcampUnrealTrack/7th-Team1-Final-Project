// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"
#include "GA_EnemyAttackFlame.generated.h"

class ANSBossAIController;
class UNSEnemyCosmeticComponent;
class UNSEnemyPartComponent;
struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.06
 *
 * 클래스 개요 : Enemy Part Row의 화염 방사 포인트를 기준으로 Cone 범위 데미지를 반복 적용하는 지속형 공격 Ability
*/
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackFlame : public UGA_EnemyAttackBase
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackFlame();

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
	struct FNSFlameEmitter
	{
		// 화염 판정 시작 위치
		FVector Start = FVector::ZeroVector;

		// 화염 판정 진행 방향
		FVector Direction = FVector::ForwardVector;
	};

	// 현재 Avatar의 CurrentAttackRow를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 현재 Avatar의 EnemyPartComponent를 반환하는 함수
	UNSEnemyPartComponent* GetEnemyPartComponent() const;

	// 현재 Avatar를 제어하는 BossAIController를 반환하는 함수
	ANSBossAIController* GetBossController() const;

	// 현재 공격 Row 기준 화염 방사 포인트 목록을 반환하는 함수
	void GetCurrentFlameEmitters(TArray<FNSFlameEmitter>& OutEmitters) const;

	// 화염 지속 판정을 시작하는 함수
	void StartFlame();

	// 화염 데미지 Tick을 처리하는 함수
	void TickFlameDamage();

	// 화염 지속 시간이 끝났을 때 Ability를 정상 종료하는 함수
	void CompleteFlame();

	// 화염 관련 Timer를 정리하는 함수
	void ClearFlameTimers();

	// 현재 유효 사거리 안에 들어온 대상만 수집하는 함수
	void CollectTargetsForEmitter(
		const FNSFlameEmitter& Emitter,
		float EffectiveRange,
		TSet<AActor*>& OutTargets) const;


	// 대상 위치가 현재 유효 사거리의 화염 원뿔 안에 있는지 확인합니다.
	bool IsLocationInsideCone(
		const FVector& Origin,
		const FVector& Direction,
		const FVector& TargetLocation,
		const FNSEnemyAttackRow& AttackRow,
		float EffectiveRange) const;

	// TargetActor가 데미지를 받을 수 있는 유효한 적대 대상인지 확인하는 함수
	bool IsValidDamageTarget(AActor* TargetActor) const;

	// TargetActor의 Bounds 기준 판정 위치를 반환하는 함수
	FVector GetTargetCheckLocation(AActor* TargetActor) const;

	// Source BaseDamage와 AttackRow DamageScale 기준 Tick 데미지를 계산하는 함수
	float CalculateFlameDamage(const FNSEnemyAttackRow& AttackRow) const;

	// TargetActor에게 Flame DamageEffect를 적용하는 함수
	bool ApplyFlameDamageToTarget(AActor* TargetActor, const FNSEnemyAttackRow& AttackRow);

	// 현재 공격 대상 Actor를 반환하는 함수
	AActor* ResolveAttackActor() const;

	// AimMode와 타깃 상태 기준으로 화염 조준 위치를 계산하는 함수
	FVector ResolveAimPoint(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// Muzzle 위치와 공격 대상 기준으로 Flame 진행 방향을 계산하는 함수
	FVector ResolveFlameDirection(
		const FNSEnemyAttackRow& AttackRow,
		const FTransform& MuzzleTransform,
		const AActor* AttackActor) const;

	// DebugData 기준으로 Flame Cone을 표시하는 함수
	void DrawDebugFlameCone(
		const FNSFlameEmitter& Emitter,
		const FNSEnemyAttackRow& AttackRow) const;

private:
	// Flame Overlap에 사용할 Trace Channel
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame")
	TEnumAsByte<ECollisionChannel> FlameTraceChannel = NSCollisionChannels::EnemyWeaponTrace;

	// 현재 Ability에서 사용할 AttackRow
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// Flame Tick 반복 Timer
	FTimerHandle FlameTickTimerHandle;

	// Flame 종료 Timer
	FTimerHandle FlameEndTimerHandle;

protected:
	// Flame 코스메틱 Start/Update/Stop을 묶기 위한 InstanceId 변수
	int32 FlameCosmeticInstanceId = INDEX_NONE;

	// Flame 코스메틱 갱신 타이머 핸들을 저장하는 변수
	FTimerHandle FlameCosmeticUpdateTimerHandle;

	// Flame 코스메틱 갱신 간격을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame|Cosmetic", meta = (ClampMin = "0.01"))
	float FlameCosmeticUpdateInterval = 0.03f;

	// 현재 Avatar의 EnemyCosmeticComponent를 반환하는 함수
	UNSEnemyCosmeticComponent* GetEnemyCosmeticComponent() const;

	// 화염 시작 코스메틱 이벤트를 클라이언트에 전송하는 함수
	void SendFlameStartCosmeticEvent() const;

	// 화염 갱신 코스메틱 이벤트를 클라이언트에 전송하는 함수
	void SendFlameUpdateCosmeticEvent(const TArray<FNSFlameEmitter>& Emitters, float EffectiveRange) const;

	// 화염 종료 코스메틱 이벤트를 클라이언트에 전송하는 함수
	void SendFlameStopCosmeticEvent() const;

	// 화염 코스메틱 이벤트 데이터를 구성하는 함수
	void BuildFlameCosmeticEvent(
		FNSCosmeticEventNetData& OutEventData,
		FGameplayTag EventTag,
		ENSCosmeticEventPhase Phase,
		const TArray<FNSFlameEmitter>& Emitters,
		float EffectiveRange) const;

	// 화염 코스메틱 위치와 범위를 클라이언트에 갱신 요청하는 함수
	void TickFlameCosmeticUpdate();

	float GetFlameRange(const FNSEnemyAttackRow& AttackRow) const;

protected:
	// 화염 유효 사거리가 0에서 AreaData.Range까지 커지는 데 걸리는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Flame", meta = (ClampMin = "0.0"))
	float FlameRangeGrowDuration = 0.35f;

	// 현재 화염 공격이 시작된 월드 시간
	float FlameStartTime = 0.0f;

	// 현재 피해 판정과 VFX에 적용되는 화염 유효 사거리
	float FlameCurrentRange = 0.0f;

	// 현재 시간 기준으로 0~1 사이의 화염 사거리 성장 비율을 반환하는 함수
	float GetFlameRangeAlpha() const;

	// AreaData.Range를 기준으로 현재 적용해야 할 화염 유효 사거리를 반환하는 함수
	float GetCurrentFlameRange() const;
};
