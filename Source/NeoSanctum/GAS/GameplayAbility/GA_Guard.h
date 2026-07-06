// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_BarrierBase.h"
#include "GA_Guard.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class AActor;

UCLASS()
class NEOSANCTUM_API UGA_Guard : public UGA_BarrierBase
{
	GENERATED_BODY()

public:
	UGA_Guard();

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

private:
	// 몽타주 Notify로 전달된 Guard 이벤트 수신
	UFUNCTION()
	void OnGuardEventReceived(FGameplayEventData Payload);

	// Guard 몽타주 정상 종료 처리
	UFUNCTION()
	void OnGuardMontageCompleted();

	// Guard 몽타주 중단 처리
	UFUNCTION()
	void OnGuardMontageInterrupted();

	// Guard Barrier 파괴 시 몽타주 섹션을 종료 섹션으로 점프
	UFUNCTION()
	void OnGuardBarrierDestroyed(AActor* DestroyedActor);

	// Barrier 스폰 이벤트 대기
	void StartGuardEventTask();

	// Guard 몽타주 재생
	bool PlayGuardMontage();

	// 입력 해제 시 종료 섹션으로 전환
	bool TryJumpToGuardEndSection() const;

	// Barrier 스탯 조회
	bool TryResolveBarrierStats(float& OutBarrierRadius, float& OutBarrierDuration) const;

	// Guard Barrier 스폰
	void SpawnGuardBarrier();

	// Guard 상태태그 부여
	void AddGuardStateTags();

	// Guard 상태태그 제거
	void RemoveGuardStateTags();

	// 이동속도 감소 GE 적용
	void ApplyGuardMoveSpeedEffect();

	// 이동속도 감소 GE 제거
	void RemoveGuardMoveSpeedEffect();

	// 활성화된 Barrier를 직접 파괴하는 헬퍼
	void DestroyActiveGuardBarrier();

protected:
	// Guard 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Guard|Animation")
	TObjectPtr<UAnimMontage> GuardMontage;

	// Guard 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Guard|Animation", meta = (ClampMin = "0.01"))
	float GuardMontagePlayRate = 1.0f;

	// 입력 해제 시 이동할 종료 섹션
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Guard|Animation")
	FName GuardEndSectionName = TEXT("End");

	// Guard 중 적용할 이동속도 감소 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Guard")
	TSubclassOf<UGameplayEffect> GuardMoveSpeedEffectClass;

	// 종료 시 Barrier를 함께 제거할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Vanguard|Guard")
	bool bDestroyBarrierOnEnd = true;

private:
	// Guard 몽타주 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> GuardMontageTask;

	// Guard 이벤트 대기 Task
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> GuardEventTask;

	// 현재 Guard Barrier
	UPROPERTY(Transient)
	TObjectPtr<ANSBarrierBase> ActiveGuardBarrier;

	// 이동속도 감소 GE 핸들
	FActiveGameplayEffectHandle GuardMoveSpeedEffectHandle;

	// 활성화 시점의 Barrier 반지름
	float CachedBarrierRadius = 0.0f;

	// 활성화 시점의 Barrier 지속시간
	float CachedBarrierDuration = 0.0f;

	// Guard 상태태그 추가 여부
	bool bGuardStateTagsAdded = false;

	// 입력 해제 종료 처리 중인지 여부
	bool bGuardEndingFromInputRelease = false;
};
