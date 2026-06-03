// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_ThrowProjectile.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * Projectile을 포물선 형태로 던지기 위한 GA
 * Input이 Hold되는 동안 궤적 + 탄착지점 프리뷰를 보여주고 
 * Input이 Release 되는 순간 프리뷰를 제거하고 Projectile을 스폰
 */
UCLASS(Abstract, Blueprintable)
class NEOSANCTUM_API UGA_ThrowProjectile : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_ThrowProjectile();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UFUNCTION()
	void OnThrowMontageCompleted();

	UFUNCTION()
	void OnThrowMontageInterrupted();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	TObjectPtr<UAnimMontage> AnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	FName ReleaseSectionName = TEXT("Release");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Throw|Montage")
	float MontagePlayRate = 1.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ThrowMontageTask;
};
