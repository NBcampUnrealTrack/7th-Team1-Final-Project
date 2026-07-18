// Copyright 2026 One Team. All rights reserved.


#include "GA_MotherShipHomingMissile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/HelperActor/NSHomingMissile.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_MotherShipHomingMissile::UGA_MotherShipHomingMissile()
{
	FGameplayTagContainer HomingAssetTags = GetAssetTags();
	HomingAssetTags.AddTag(NSGameplayTags::Ability_Enemy_MotherShip_HomingMissile);
	SetAssetTags(HomingAssetTags);
	
	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_MotherShipHomingMissile::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (PendingDelayTask)
	{
		PendingDelayTask->EndTask();
		PendingDelayTask = nullptr;
	}
	CachedAttackRow = nullptr;
	bVolleyStarted = false;
	FiredShotCount = 0;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MotherShipHomingMissile::InitializeAttack()
{
	Super::InitializeAttack();
	
	CachedAttackRow = GetCurrentAttackRow();
	bVolleyStarted = false;
	FiredShotCount = 0;
}

void UGA_MotherShipHomingMissile::HandleAttackEvent(const FGameplayEventData& Payload)
{
	if (bVolleyStarted)
	{
		return;
	}

	bVolleyStarted = true;
	FireOneShot();
}

void UGA_MotherShipHomingMissile::HandleAttackMontageCompleted()
{
	
}

void UGA_MotherShipHomingMissile::FireOneShot()
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor || !SourceActor->HasAuthority() || !CachedAttackRow)
	{
		CancelAttackAbility();
		return;
	}
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		CancelAttackAbility();
		return;
	}
	
	AActor* Target = FindNearestKnownTarget(Cast<APawn>(SourceActor));
	if (!IsValid(Target))
	{
		ANSBossAIController* BossAIC = GetBossController();
		if (!BossAIC)
		{
			CancelAttackAbility();
			return;
		}

		Target = BossAIC->GetCurrentAttackActor();
		if (!Target)
		{
			Target = BossAIC->GetCurrentTargetActor();
		}
	}

	if (!IsValid(Target))
	{
		CancelAttackAbility();
		return;
	}
	
	UNSEnemyPartComponent* SourcePart = GetEnemyPartComponent();
	if (!SourcePart) 
	{
		CancelAttackAbility();
		return;
	}
	
	TArray<FTransform> Muzzles;
	SourcePart->GetMuzzleTransformsByAttackId(CachedAttackRow->AttackId, Muzzles);
	if (Muzzles.IsEmpty())
	{
		CancelAttackAbility();
		return;
	}
	
	float Damage = CalculateMissileDamage(*CachedAttackRow);
	float ExplosionRadius = CachedAttackRow->AreaData.Radius;
	
	TArray<AActor*> IgnoredPartActors;
	SourcePart->GetSpawnedPartActorsByAttackId(CachedAttackRow->AttackId, IgnoredPartActors);
	
	for (FTransform& Muzzle : Muzzles)
	{
		FVector Dir = (Target->GetActorLocation() - Muzzle.GetLocation()).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			Dir = Muzzle.GetRotation().GetForwardVector();
		}
		
		if (FireCueTag.IsValid())
		{
			FGameplayCueParameters FireCueParams;
			FireCueParams.Instigator = SourceActor;
			FireCueParams.EffectCauser = SourceActor;
			FireCueParams.Location = Muzzle.GetLocation();
			FireCueParams.Normal = Dir;
			SourceASC->ExecuteGameplayCue(FireCueTag, FireCueParams);
		}
		
		ANSHomingMissile* Missile = GetWorld()->SpawnActorDeferred<ANSHomingMissile>(
			MissileClass,
			Muzzle,
			SourceActor,
			Cast<APawn>(SourceActor)
			);
		
		Muzzle.SetScale3D(FVector::OneVector);
		
		if (Missile)
		{
			Missile->InitializeMissile(
				SourceASC,
				DamageEffectClass,
				Damage,
				ExplosionRadius,
				Target->GetRootComponent(),
				IgnoredPartActors
				);
			
			Missile->FinishSpawning(Muzzle);
			Missile->LaunchMissile(Dir);
		}
	}
	
	++FiredShotCount;
	if(FiredShotCount >= CachedAttackRow->ProjectileData.FireCount)
	{
		FinishAttackAbility();
		return;
	}
	else
	{
		ScheduleNextShot();
	}
}

void UGA_MotherShipHomingMissile::ScheduleNextShot()
{
	PendingDelayTask = UAbilityTask_WaitDelay::WaitDelay(
		this,
		CachedAttackRow->
		ProjectileData.FireInterval);
	
	if (!PendingDelayTask)
	{
		FinishAttackAbility();
		return;
	}
	PendingDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnShotDelayFinished);
	PendingDelayTask->ReadyForActivation();
}

void UGA_MotherShipHomingMissile::OnShotDelayFinished()
{
	PendingDelayTask = nullptr;
	FireOneShot();
}

const FNSEnemyAttackRow* UGA_MotherShipHomingMissile::GetCurrentAttackRow() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AvatarActor);
	return EnemyAgent ? EnemyAgent->GetCurrentAttackRow() : nullptr;
}

UNSEnemyPartComponent* UGA_MotherShipHomingMissile::GetEnemyPartComponent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<UNSEnemyPartComponent>() : nullptr;
}

ANSBossAIController* UGA_MotherShipHomingMissile::GetBossController() const
{
	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	return AvatarPawn ? Cast<ANSBossAIController>(AvatarPawn->GetController()) : nullptr;
}

float UGA_MotherShipHomingMissile::CalculateMissileDamage(const FNSEnemyAttackRow& AttackRow) const
{
	UAbilitySystemComponent* SourceASC =GetAbilitySystemComponentFromActorInfo();
	return FMath::Max(SourceASC->GetNumericAttribute(UNSBaseAttributeSet::GetBaseDamageAttribute()) * AttackRow.DamageScale,0.0f);
}
