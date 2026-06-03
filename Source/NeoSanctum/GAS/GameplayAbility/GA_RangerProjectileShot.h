// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_RangerProjectileShot.generated.h"

class ANSRangerProjectile;

/**
 * Ranger 투사체 발사 Ability
 * GA는 서버에서 Projectile을 스폰하고 이후 이동 및 충돌은 Projectile Actor가 담당.
 */

UCLASS()
class NEOSANCTUM_API UGA_RangerProjectileShot : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_RangerProjectileShot();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Ranger|Projectile")
	TSubclassOf<ANSRangerProjectile> ProjectileClass;
	
private:
	bool TrySpawnProjectile() const;
	bool TryGetAttackOriginTransform(FTransform& OutTransform) const;
};
