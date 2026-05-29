// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerAutoFire.generated.h"

class UGameplayEffect;

/**
 * 원거리 캐릭터 기본 공격
 * 입력 유지 중 반복 활성화 되는 히트스캔 공격 반복
 */
UCLASS()
class NEOSANCTUM_API UGA_RangerAutoFire : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_RangerAutoFire();

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
	
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float FireInterval = 0.15f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float TraceRange = 10000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugHitscan = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineDuration = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	float DebugLineThickness = 1.5f;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bLogPredictionKey = true;
	
private:
	void FinishFireCycle();

	void FireOnce();
	
	FGameplayAbilityTargetDataHandle MakeTargetDataFromHitResult(const FHitResult& HitResult) const;
	void OnTargetDataReadyCallback(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ApplicationTag);
	void OnRangerTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	void ProcessTargetDataForDamage(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	void ApplyDamageToActor(AActor* TargetActor);

	void PlayFireFeedback();
	void ExecuteMuzzleFireCue();
	void DrawDebugHitscan();

	bool ShouldPlayLocalFeedback() const;
	bool TryBuildHitscanTrace(
		FHitResult& OutHitResult,
		FVector& OutTraceStart,
		FVector& OutTraceEnd,
		bool& bOutHit) const;
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
	
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
	FTimerHandle FireDelayTimerHandle;
};
