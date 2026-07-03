// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackRanger.h"

#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileManagerComponent.h"
#include "NeoSanctum/Combat/Projectile/NSProjectileTypes.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
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

	AActor* AttackActor = AIController ? AIController->GetCurrentAttackActor() : nullptr;
	if (!IsValid(AttackActor))
	{
		AttackActor = AIController ? AIController->GetCurrentTargetActor() : nullptr;
	}

	if (!IsValid(AttackActor))
	{
		return;
	}

	Enemy->UpdateCombatAimTarget(AttackActor);

	const FNSEnemyAttackRow* CurrentAttackRow = Enemy->GetCurrentAttackRow();
	if (!CurrentAttackRow)
	{
		return;
	}

	const UNSEnemyPartComponent* PartComponent = Enemy->FindComponentByClass<UNSEnemyPartComponent>();

	FTransform MuzzleTransform;
	if (!PartComponent ||
		!PartComponent->TryGetMuzzleTransformByAttackId(
			CurrentAttackRow->AttackId,
			MuzzleTransform))
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
	const FVector AimLocation = Enemy->HasCombatAimTarget()
		                            ? Enemy->GetCombatAimTargetLocation()
		                            : AttackActor->GetActorLocation();

	FVector Direction = (AimLocation - StartLocation).GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		Direction = MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();
	}

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
	Request.DamageEffectClass = DamageEffectClass;

	ProjectileManager->FireProjectile(Request);
}
