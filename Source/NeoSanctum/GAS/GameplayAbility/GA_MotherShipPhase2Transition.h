// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MotherShipPhase2Transition.generated.h"

class UAnimMontage;

UCLASS()
class NEOSANCTUM_API UGA_MotherShipPhase2Transition : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_MotherShipPhase2Transition();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	// 연출(대기/몽타주) 종료 시 호출
	UFUNCTION()
	void OnTransitionSequenceFinished();

	// ---- Config ----
	// 연출 지속 시간(초). 몽타주 붙기 전 임시 무적 유지 시간
	UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition", meta = (ClampMin = "0.0"))
	float TransitionDuration = 5.f;

	// (추후) 전환 연출 몽타주. 지정 시 WaitDelay 대신 PlayMontageAndWait 사용
	UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition")
	TObjectPtr<UAnimMontage> TransitionMontage;
};