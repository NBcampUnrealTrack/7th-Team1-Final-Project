// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_EngineerBarrier.generated.h"

class ANSBarrier;
/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UGA_EngineerBarrier : public UGA_SkillBase
{
	GENERATED_BODY()
	
public:
	UGA_EngineerBarrier();
	
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
	
	virtual void ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo
) const override;
	
private:
	bool TryGetFinalCooldownDuration(float& OutCooldownDuration) const;
	bool TryGetBarrierRadius(float& OutBarrierRadius) const;
	void SpawnBarrierActor(const FGameplayAbilityActorInfo* ActorInfo, float BarrierRadius);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	TSubclassOf<ANSBarrier> BarrierClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FGameplayTag AbilityTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FGameplayTag CooldownEffectTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	float MinimumBarrierRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FTransform AttachRelativeTransform = FTransform::Identity;

private:
	UPROPERTY(Transient)
	TObjectPtr<ANSBarrier> ActiveBarrier;
};
