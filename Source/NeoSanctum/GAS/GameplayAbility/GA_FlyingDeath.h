#pragma once

#include "CoreMinimal.h"
#include "GA_EnemyDeath.h"
#include "GA_FlyingDeath.generated.h"

class UNSFlyingLocomotionComponent;

UCLASS()
class NEOSANCTUM_API UGA_FlyingDeath : public UGA_EnemyDeath
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UNSFlyingLocomotionComponent* GetAvatarFlyingLocomotion() const;
};