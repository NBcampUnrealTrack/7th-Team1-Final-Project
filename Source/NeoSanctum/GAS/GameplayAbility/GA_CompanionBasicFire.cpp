// Copyright 2026 One Team. All rights reserved.


#include "GA_CompanionBasicFire.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"

UGA_CompanionBasicFire::UGA_CompanionBasicFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	AbilityTags.AddTag(NSGameplayTags::Ability_Companion_Fire);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Companion_Disable);
	
	DamageSetTag = NSGameplayTags::Data_Companion_Damage;
	CoolDownTag = NSGameplayTags::Data_Companion_CoolDown;
}

void UGA_CompanionBasicFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!IsValid(GetCombatTarget()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	
}

void UGA_CompanionBasicFire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

const UNSCompanionAttributeSet* UGA_CompanionBasicFire::GetCompanionSet() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return nullptr;
	
	return ASC->GetSet<UNSCompanionAttributeSet>();
}

AActor* UGA_CompanionBasicFire::GetCombatTarget() const
{
	ANSBaseCompanionAI* DronePawn = Cast<ANSBaseCompanionAI>(GetAvatarActorFromActorInfo());
	if (!DronePawn) return nullptr;
	
	ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>(DronePawn->GetController());
	if (!DroneAIController) return nullptr;
	
	UBlackboardComponent* BB = DroneAIController->GetBlackboardComponent();
	if (!BB) return nullptr;
	
	return Cast<AActor>(BB->GetValueAsObject(CombatTargetKeyName));
}

bool UGA_CompanionBasicFire::CanFireAt(AActor* Target, FVector& OutMuzzle, FVector& OutDir) const
{
	if (!Target) return false;
	
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return false;
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return false;
	
	OutMuzzle = AvatarActor->GetActorLocation();
	
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ToTarget = TargetLocation - OutMuzzle;
	OutDir = ToTarget.GetSafeNormal();
	
	const float DistToTarget = ToTarget.Size();
	if (DistToTarget > Set->GetAttackRange()) return false;
	
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(AvatarActor);
	
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		OutMuzzle,
		TargetLocation,
		ECC_Visibility,
		CollisionParams);
	
	if (bHit && Hit.GetActor() != Target) return false;
	
	return true;
}

void UGA_CompanionBasicFire::FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target)
{
}
