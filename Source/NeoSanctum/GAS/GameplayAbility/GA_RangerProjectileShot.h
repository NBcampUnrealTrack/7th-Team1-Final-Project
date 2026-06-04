// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerProjectileShot.generated.h"

class ANSRangerProjectile;

/**
 * Ranger 투사체 발사 Ability
 * GA는 서버에서 Projectile을 스폰하고 이후 이동 및 충돌은 Projectile Actor가 담당.
 */
UCLASS()
class NEOSANCTUM_API UGA_RangerProjectileShot : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_RangerProjectileShot();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Ranger|Projectile")
	TSubclassOf<ANSRangerProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Ranger|Projectile")
	float TraceRange = 10000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Ranger|Projectile")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bKeepAbilityActiveForDebug = false;

	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly, Category = "GAS|Debug",
		meta = (EditCondition = "bKeepAbilityActiveForDebug"))
	float DebugActiveDuration = 1.0f;

private:
	void FireProjectileShot();
	
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult) const;
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		FGameplayTag ApplicationTag
	);
	
	void OnProjectileTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	bool TryBuildProjectileAimTrace(FHitResult& OutHitResult) const;
	bool TrySpawnProjectileFromTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle) const;
	bool TrySpawnProjectileAtAimPoint(const FVector& AimPoint) const;
	
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
	
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
	
private:
	void FinishDebugAbility();

	FTimerHandle DebugEndAbilityTimerHandle;
};
