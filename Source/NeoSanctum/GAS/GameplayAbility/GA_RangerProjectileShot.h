// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerProjectileShot.generated.h"

class ANSRangerProjectile;
class UAnimMontage;

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
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Animation")
	TObjectPtr<UAnimMontage> FireMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger|Animation")
	float FireMontagePlayRate = 1.0f;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bKeepAbilityActiveForDebug = false;

	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "GAS|Debug",
		meta = (EditCondition = "bKeepAbilityActiveForDebug"))
	float DebugActiveDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugProjectileAimTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugProjectileLaunch = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineThickness = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugPointSize = 12.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugAimTraceStartOffset = 200.0f;

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
	
	void PlayFireMontage();

private:
	// TargetData
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;

	bool TryGetAimPointFromTargetData(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		FVector& OutAimPoint
	) const;
	
	// GameplayCue
	void ExecuteProjectileMuzzleCue(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

private:
	// Debug
	void DrawDebugProjectileAimTrace(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FHitResult& HitResult,
		bool bHit
	) const;

	void DrawDebugProjectileLaunch(
		const FVector& MuzzleLocation,
		const FVector& AimPoint,
		const FColor& DebugColor
	) const;


	void FinishDebugAbility();

	FTimerHandle DebugEndAbilityTimerHandle;
};
