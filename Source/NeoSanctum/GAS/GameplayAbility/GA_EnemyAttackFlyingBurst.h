// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "GA_EnemyAttackFlyingBurst.generated.h"

class UAbilityTask_WaitDelay;
struct FNSEnemyAttackRow;

UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackFlyingBurst : public UGA_EnemyAttackBase
{
	GENERATED_BODY()
	
public:
	UGA_EnemyAttackFlyingBurst();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 부모의 몽타주 완료 → 자동 Finish를 막기 위해 no-op으로 오버라이드
	virtual void HandleAttackMontageCompleted() override;
	
	// 볼리 1회(좌·우 소켓 동시) 발사. 검증 실패 시 CancelAttackAbility.
	void FireOneVolley();

	// 다음 볼리를 ShotInterval 뒤에 예약
	void ScheduleNextVolley();

	// 데미지 계산 함수
	float CalculateProjectileDamage(const FNSEnemyAttackRow& AttackRow) const;
	
	UFUNCTION()
	void OnShotDelayFinished();

protected:
	// 발사 볼리 수 (드론 BP: 3, 보스 BP: 5)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Burst", meta = (ClampMin = "1"))
	int32 BurstCount = 3;

	// 볼리 간 간격 (드론 BP: 0.5, 보스 BP: 더 짧게)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Burst", meta = (ClampMin = "0.0"))
	float ShotInterval = 0.5f;

	// --- 투사체 파라미터 (Ranger와 동일) ---
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "0.01"))
	float ProjectileMaxLifeTime = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileRadius = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	TEnumAsByte<ECollisionChannel> ProjectileTraceChannel = NSCollisionChannels::EnemyProjectile;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cue")
	FGameplayTag EnemyDroneFireCueTag;
	
private:
	// 지금까지 발사한 볼리 수
	int32 CurrentVolley = 0;

	// 예약된(아직 발동 전) 지연 태스크. Abort 시 정리 대상.
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> PendingDelayTask;
};