// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackMelee.h"

#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "DrawDebugHelpers.h"

UGA_EnemyAttackMelee::UGA_EnemyAttackMelee()
{
	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_MeleeAttack);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackMelee::InitializeAttack()
{
	bHasHitThisAttack = false;

	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Enemy))
	{
		return;
	}

	if (const FNSEnemyAttackDefinition* CurrentAttackDefinition = Enemy->GetCurrentAttackDefinition())
	{
		AttackTraceDistance = CurrentAttackDefinition->Condition.MaxRange;
		AttackTraceRadius = CurrentAttackDefinition->MeleeTraceRadius;
	}
}

void UGA_EnemyAttackMelee::PrepareForAttackMontage()
{
	bHasHitThisAttack = false;
}

void UGA_EnemyAttackMelee::HandleAttackEvent(const FGameplayEventData& Payload)
{
	if (bHasHitThisAttack)
	{
		return;
	}

	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(GetAvatarActorFromActorInfo());

	UWorld* World = GetWorld();

	if (!IsValid(Enemy) || !IsValid(World))
	{
		return;
	}

	FVector Start = Enemy->GetActorLocation();
	FVector End = Start +
		Enemy->GetActorForwardVector() * AttackTraceDistance;

	ANSEnemyWeaponBase* Weapon = Enemy->GetCurrentWeapon();

	if (IsValid(Weapon))
	{
		USkeletalMeshComponent* WeaponMesh = Weapon->GetComponentByClass<USkeletalMeshComponent>();

		if (IsValid(WeaponMesh) &&
			WeaponMesh->DoesSocketExist(TEXT("TraceStart")) &&
			WeaponMesh->DoesSocketExist(TEXT("TraceEnd")))
		{
			Start = WeaponMesh->GetSocketLocation(TEXT("TraceStart"));
			End = WeaponMesh->GetSocketLocation(TEXT("TraceEnd"));
		}
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Enemy);

	if (IsValid(Weapon))
	{
		QueryParams.AddIgnoredActor(Weapon);
	}

	FHitResult HitResult;

	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AttackTraceRadius),
		QueryParams);

	if (bHit && TryApplyDamageToTarget(HitResult.GetActor(), HitResult))
	{
		bHasHitThisAttack = true;
	}
	
	DrawAttackTraceDebug(Start, End, bHit, HitResult);
}

void UGA_EnemyAttackMelee::DrawAttackTraceDebug(
	const FVector& Start,
	const FVector& End,
	bool bHit,
	const FHitResult& HitResult) const
{
	if (!bDrawAttackTraceDebug)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor TraceColor = bHit ? FColor::Red : FColor::Green;
	const FColor CenterLineColor = FColor::White;

	DrawDebugSphere(
		World,
		Start,
		AttackTraceRadius,
		16,
		TraceColor,
		false,
		AttackTraceDebugDuration);

	DrawDebugSphere(
		World,
		End,
		AttackTraceRadius,
		16,
		TraceColor,
		false,
		AttackTraceDebugDuration);

	DrawDebugLine(
		World,
		Start,
		End,
		CenterLineColor,
		false,
		AttackTraceDebugDuration,
		0,
		1.5f);

	const FVector SweepVector = End - Start;
	const float SweepLength = SweepVector.Size();

	if (SweepLength > KINDA_SMALL_NUMBER)
	{
		const FVector SweepDirection = SweepVector / SweepLength;
		const FVector CapsuleCenter = (Start + End) * 0.5f;

		const float CapsuleHalfHeight = (SweepLength * 0.5f) + AttackTraceRadius;

		const FQuat CapsuleRotation =
			FRotationMatrix::MakeFromZ(SweepDirection).ToQuat();

		DrawDebugCapsule(
			World,
			CapsuleCenter,
			CapsuleHalfHeight,
			AttackTraceRadius,
			CapsuleRotation,
			TraceColor,
			false,
			AttackTraceDebugDuration,
			0,
			1.5f);
	}

	if (bHit)
	{
		DrawDebugPoint(
			World,
			HitResult.ImpactPoint,
			12.0f,
			FColor::Yellow,
			false,
			AttackTraceDebugDuration);

		DrawDebugLine(
			World,
			HitResult.TraceStart,
			HitResult.ImpactPoint,
			FColor::Yellow,
			false,
			AttackTraceDebugDuration,
			0,
			2.0f);
	}
}
