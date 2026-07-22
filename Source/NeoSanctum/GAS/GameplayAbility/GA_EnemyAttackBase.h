// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "NeoSanctum/Core/Interface/NSHitReactionSourceInterface.h"
#include "GA_EnemyAttackBase.generated.h"

class UNSEnemyData;

UENUM(BlueprintType)
enum class ENSPatternDefenseWindow : uint8
{
	None,
	AbilityLifetime,
	AttackMontage
};

/**
 * 모든 적 AI 공격 어빌리티의 최상위 부모 C++ 클래스
 * 멀티플레이어 애니메이션 동기화 및 BT 제어권 연동 흐름을 통제합니다.
 */
UCLASS()
class NEOSANCTUM_API UGA_EnemyAttackBase : public UGameplayAbility,
                                           public INSHitReactionSourceInterface
{
	GENERATED_BODY()

public:
	UGA_EnemyAttackBase();

	virtual ENSHitReactionAttackType GetHitReactionAttackType() const override { return ENSHitReactionAttackType::Any; }

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	bool PlayAttackMontage();
	void FinishAttackAbility();
	void CancelAttackAbility();
	void BeginPatternDefenseWindow(ENSPatternDefenseWindow Window);
	void EndPatternDefenseWindow(ENSPatternDefenseWindow Window);

	virtual void InitializeAttack();
	virtual void PrepareForAttackMontage();
	virtual void HandleAttackMontageCompleted();
	virtual void HandleAttackEvent(const FGameplayEventData& Payload);

	bool TryApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult);

	// @민재 : 공격 주체 폰의 ThreatComponent에서 가장 가까운 known 타겟을 찾는다. 후보 없으면 nullptr.
	AActor* FindNearestKnownTarget(const APawn* AttackerPawn) const;
	
private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnMontageBlendOut();
	
	void ApplyPatternDefenseEffect();
	void RemovePatternDefenseEffect();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Assets")
	FGameplayTag GameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Config")
	FGameplayTag HitCheckEventTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Pattern Defense")
	ENSPatternDefenseWindow PatternDefenseWindow = ENSPatternDefenseWindow::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Pattern Defense")
	TSubclassOf<UGameplayEffect> PatternDefenseEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Pattern Defense", meta = (ClampMin = "0.0"))
	float PatternDefenseBonus = 0.0f;
	
private:
	FActiveGameplayEffectHandle PatternDefenseEffectHandle;
};
