// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerAutoFire.generated.h"

/**
 * 원거리 캐릭터 기본 공격
 * 입력 유지 중 히트스캔 공격 반복
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
	
	UFUNCTION()
	void OnInputReleased(float TimeHeld);
	
	void StartAutoFire();
	void StopAutoFire();
	
	void FireOnce();
	void PerformHitscan();
	void ApplyDamageToActor(AActor* TargetActor);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float FireInterval = 0.15f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	float TraceRange = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ranger")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	
private:
	FTimerHandle AutoFireTimerHandle;
};
