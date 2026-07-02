// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnemyDeath.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

/**
 * Enemy 사망 연출을 처리하는 Ability입니다.
 * DeathMontage가 있으면 몽타주를 재생하고, 없으면 Enemy Mesh를 래그돌로 전환합니다.
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyDeath : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyDeath();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 몬스터별로 재생할 사망 몽타주. 없으면 Ragdoll 실행.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 사망 몽타주의 재생 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	// 재생할 몽타주 섹션. None이면 기본 섹션을 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Animation")
	FName StartSectionName = NAME_None;

	// 사망 연출 이후 DissolveComponent가 있으면 디졸브를 시작할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Dissolve")
	bool bStartDissolve = true;

	// 디졸브 완료 후 Actor를 Destroy할지 여부. 풀링 몬스터는 false로 둡니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Dissolve")
	bool bDestroyAfterDissolve = false;

private:
	// DeathMontage가 정상 종료되었을 때 Ability를 종료하는 함수
	UFUNCTION()
	void OnDeathMontageCompleted();

	// DeathMontage가 중단되거나 실패했을 때 래그돌 fallback 후 Ability를 종료하는 함수
	UFUNCTION()
	void OnDeathMontageInterrupted();

	// EnemyAgent 또는 Actor Component 기준으로 사망 연출에 사용할 Mesh를 찾는 함수
	USkeletalMeshComponent* GetDeathMesh() const;

	// Character/Pawn의 루트 충돌과 이동을 사망 상태에 맞게 정리하는 함수
	void DisableDeathCollision(AActor* AvatarActor, USkeletalMeshComponent* MeshComponent) const;

	// Mesh를 래그돌 상태로 전환하는 함수
	bool ApplyRagdoll(USkeletalMeshComponent* MeshComponent) const;

	// DissolveComponent가 있으면 디졸브를 시작하는 함수
	void StartDissolve() const;

	// 사망 Ability를 종료하는 공통 함수
	void FinishDeathAbility(bool bWasCancelled);
	
	void FreezeDeathPose();
	
private:
	FTimerHandle DeathFreezeTimerHandle;
};
