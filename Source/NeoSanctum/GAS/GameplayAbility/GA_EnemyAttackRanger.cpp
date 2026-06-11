// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackRanger.h"

#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileTypes.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_EnemyAttackRanger::UGA_EnemyAttackRanger()
{
	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_RangerAttack);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackRanger::InitializeAttack()
{
	CurrentShotCount = 0;
}

void UGA_EnemyAttackRanger::HandleAttackMontageCompleted()
{
	Super::HandleAttackMontageCompleted();
}

void UGA_EnemyAttackRanger::HandleAttackEvent(const FGameplayEventData& Payload)
{
	if (CurrentShotCount >= BurstCount)
	{
		return;
	}

	++CurrentShotCount;

	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		const FName NextSection = CurrentShotCount < BurstCount ? FName(TEXT("Fire")) : NAME_None;

		AnimInstance->Montage_SetNextSection(
			TEXT("Fire"),
			NextSection,
			AttackMontage);
	}
	
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetAvatarActorFromActorInfo());

	// 서버에서만 실행
	if (!IsValid(Enemy) || !Enemy->HasAuthority())
	{
		return;
	}

	ANSEnemyAIController* AIController = Cast<ANSEnemyAIController>(Enemy->GetController());
	AActor* TargetActor = AIController ? AIController->GetCurrentTargetActor() : nullptr;

	if (!IsValid(TargetActor))
	{
		return;
	}

	ANSEnemyWeaponBase* Weapon = Enemy->GetCurrentWeapon();

	FTransform MuzzleTransform;
	if (!IsValid(Weapon) || !Weapon->TryGetMuzzleTransform(MuzzleTransform))
	{
		return;
	}

	UWorld* World = GetWorld();
	ANSRunGameState* RunGameState = World ? World->GetGameState<ANSRunGameState>() : nullptr;
	UNSProjectileManagerComponent* ProjectileManager =
		RunGameState ? RunGameState->GetProjectileManagerComponent() : nullptr;

	if (!ProjectileManager)
	{
		return;
	}

	const FVector StartLocation = MuzzleTransform.GetLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector::UpVector * TargetAimHeightOffset;
	const FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		return;
	}

	FNSProjectileFireRequest Request;
	Request.StartLocation = StartLocation;
	Request.Direction = Direction;
	Request.Speed = ProjectileSpeed;
	Request.MaxLifeTime = ProjectileMaxLifeTime;
	Request.Radius = ProjectileRadius;
	Request.TraceChannel = ProjectileTraceChannel;
	Request.SourceActor = Enemy;

	ProjectileManager->FireProjectile(Request);
}
