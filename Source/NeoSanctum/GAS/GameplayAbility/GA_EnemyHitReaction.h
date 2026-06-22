// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnemyHitReaction.generated.h"

class UAnimMontage;

/**
 * 피격 게이지가 최대치에 도달했을 때 공격을 취소하고
 * 피격 경직 몽타주를 재생하는 몬스터 전용 Ability입니다.
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyHitReaction : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// 서버 전용 경직 Ability 정책과 태그를 초기화하는 생성자
	UGA_EnemyHitReaction();

	// 공격을 취소하고 피격 경직 몽타주를 재생하는 함수
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Ability 종료 시 캐릭터와 Behavior Tree의 경직 상태를 복구하는 함수
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 몬스터별로 재생할 전신 피격 경직 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction|Animation")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	// 피격 경직 몽타주의 재생 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	// 재생할 몽타주 섹션으로, None이면 기본 섹션을 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Reaction|Animation")
	FName StartSectionName = NAME_None;

private:
	// 피격 경직 몽타주가 정상 종료되면 Ability를 완료하는 콜백 함수
	UFUNCTION()
	void OnHitReactionMontageCompleted();

	// 피격 경직 몽타주가 취소되거나 중단되면 Ability를 취소하는 콜백 함수
	UFUNCTION()
	void OnHitReactionMontageInterrupted();
};
