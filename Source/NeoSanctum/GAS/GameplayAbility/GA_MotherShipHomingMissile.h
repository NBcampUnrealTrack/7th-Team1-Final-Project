// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyAttackBase.h"
#include "GA_MotherShipHomingMissile.generated.h"

class ANSHomingMissile;
class ANSBossAIController;
class UNSEnemyPartComponent;
class UAbilityTask_WaitDelay;
struct FNSEnemyAttackRow;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UGA_MotherShipHomingMissile : public UGA_EnemyAttackBase
{
	GENERATED_BODY()
	
public:
	UGA_MotherShipHomingMissile();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void InitializeAttack() override;
	virtual void HandleAttackEvent(const FGameplayEventData& Payload) override;

	// FlyingBurst/BombingRun과 동일하게 no-op (몽타주 길이와 연발 수명 분리)
	virtual void HandleAttackMontageCompleted() override;

private:
	void FireOneShot();
	void ScheduleNextShot();

	UFUNCTION()
	void OnShotDelayFinished();

	const FNSEnemyAttackRow* GetCurrentAttackRow() const;
	UNSEnemyPartComponent* GetEnemyPartComponent() const;
	ANSBossAIController* GetBossController() const;

	float CalculateMissileDamage(const FNSEnemyAttackRow& AttackRow) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Missile")
	TSubclassOf<ANSHomingMissile> MissileClass;

	// 발사 시 각 Muzzle 소켓에서 실행할 발사 GameplayCue (머즐 플래시 + 발사음)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Cue")
	FGameplayTag FireCueTag;
	
	const FNSEnemyAttackRow* CachedAttackRow = nullptr;

	// 재진입 가드 (Bombard 패턴): 첫 GameplayEvent에서만 연발 루프 kick off
	bool bVolleyStarted = false;

	int32 FiredShotCount = 0;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> PendingDelayTask;
};
