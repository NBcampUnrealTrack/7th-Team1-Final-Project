// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "GA_CompanionBasicFire.generated.h"


class UNSCompanionAttributeSet;
class ANSDroneProjectile;
class UGameplayEffect;

UCLASS(Abstract)
class NEOSANCTUM_API UGA_CompanionBasicFire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_CompanionBasicFire();
	
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	// Getter
	FVector GetMuzzleSocketLocation() const;
	const UNSCompanionAttributeSet* GetCompanionSet() const;
	AActor* GetCombatTarget() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<ANSDroneProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag DamageSetTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag CoolDownTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FName MuzzleSocketName;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FName EnemyTargetKey;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag FireCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	bool bPredictiveAim = false;
	
	// @민재 : 타겟 움직임 예측 발사
	FVector ComputeAimDirection(const FVector& Muzzle, AActor* Target) const;
	
	// @민재 : 조준성공 발사위치 반환
	bool CanFireAt(AActor* Target, FVector& OutMuzzle, FVector& OutDir) const;
	
	// @민재 : 실제 탄 발사 함수
	void FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target);
};
