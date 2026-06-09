// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_CompanionBasicFire.generated.h"


class UNSCompanionAttributeSet;
class ANSDroneProjectile;
class UGameplayEffect;

UCLASS()
class NEOSANCTUM_API UGA_CompanionBasicFire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_CompanionBasicFire();
	
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
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
	FName CombatTargetKeyName;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	FGameplayTag FireCueTag;
	
	const UNSCompanionAttributeSet* GetCompanionSet() const;
	AActor* GetCombatTarget() const;
	bool CanFireAt(AActor* Target, FVector& OutMuzzle, FVector& OutDir) const;
	void FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target);
};
