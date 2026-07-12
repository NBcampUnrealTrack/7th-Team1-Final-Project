#include "GA_FlyingDeath.h"

#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"

void UGA_FlyingDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 부모(몽타주/디졸브/콜리전 처리) 전에 고도유지 끄고 낙하 시작
	if (UNSFlyingLocomotionComponent* Flying = GetAvatarFlyingLocomotion())
	{
		Flying->StartDeathFall();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UNSFlyingLocomotionComponent* UGA_FlyingDeath::GetAvatarFlyingLocomotion() const
{
	if (ANSEnemyPawnBase* EnemyPawn = Cast<ANSEnemyPawnBase>(GetAvatarActorFromActorInfo()))
	{
		return EnemyPawn->GetFlyingLocomotion();
	}
	return nullptr;
}