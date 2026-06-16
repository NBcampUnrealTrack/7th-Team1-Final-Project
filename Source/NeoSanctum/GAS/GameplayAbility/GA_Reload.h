// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_Reload.generated.h"

class UAnimMontage;

/**
 * 원거리 캐릭터 공용 재장전 Ability
 */
UCLASS()
class NEOSANCTUM_API UGA_Reload : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_Reload();

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

	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags
	) const override;

protected:
	// CombatStat 조회에 사용할 AbilityTag
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Reload")
	FGameplayTag CombatStatAbilityTag;

	// 재장전 속도 CombatStat 태그
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Reload")
	FGameplayTag ReloadSpeedStatTag;

	// 재장전 시간이 너무 작아져서 거의 알아볼 수 없는 속도로 장전되면서 유저에게 피드백을 주지 못하는 상황 방지
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Reload")
	float MinReloadDuration = 0.05f;

	// 재장전 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Reload|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	// 재장전 시간 보정 시 허용할 최대 몽타주 재생속도
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Reload|Animation")
	float MaxMontagePlayRate = 3.0f;

private:
	void FinishReload();
	void PlayReloadMontage(float ReloadDuration);
	void AddDeactivateHandIKTag();
	void RemoveDeactivateHandIKTag();
	
private:
	bool TryGetFinalReloadDuration(float& OutReloadDuration) const;
	FGameplayTag GetReloadCombatStatAbilityTag() const;

private:
	FTimerHandle ReloadTimerHandle;
	bool bDeactivateHandIKTagAdded = false;
};
