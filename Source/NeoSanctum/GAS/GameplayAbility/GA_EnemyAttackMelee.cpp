// Copyright 2026 One Team. All rights reserved.


#include "GA_EnemyAttackMelee.h"

#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "DrawDebugHelpers.h"

UGA_EnemyAttackMelee::UGA_EnemyAttackMelee()
{
	// Tags 세팅 설정
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Enemy_BasicMelee);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(NSGameplayTags::State_Enemy_Combat);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Dead);
}

void UGA_EnemyAttackMelee::InitializeAttack()
{
	DamagedTraceWindowIds.Reset();

	const AActor* EnemyActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(EnemyActor);

	if (!IsValid(EnemyActor) || !EnemyAgent)
	{
		return;
	}

	const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgent->GetCurrentAttackRow();
	if (!CurrentAttackRow)
	{
		return;
	}

	AttackTraceDistance = CurrentAttackRow->Condition.MaxRange;
	AttackTraceRadius = CurrentAttackRow->MeleeTraceRadius;
}

void UGA_EnemyAttackMelee::HandleAttackEvent(const FGameplayEventData& Payload)
{
	const UObject* TraceWindow = Payload.OptionalObject.Get();
	const uint32 TraceWindowId = IsValid(TraceWindow) ? TraceWindow->GetUniqueID() : 0;

	if (TraceWindowId != 0 && DamagedTraceWindowIds.Contains(TraceWindowId))
	{
		return;
	}

	AActor* EnemyActor = GetAvatarActorFromActorInfo();
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(EnemyActor);

	UWorld* World = GetWorld();

	if (!IsValid(EnemyActor) || !EnemyAgent || !IsValid(World))
	{
		return;
	}

	FVector Start = EnemyActor->GetActorLocation();
	FVector End = Start + EnemyActor->GetActorForwardVector() * AttackTraceDistance;

	const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgent->GetCurrentAttackRow();
	const UNSEnemyPartComponent* PartComponent = EnemyActor->FindComponentByClass<UNSEnemyPartComponent>();

	if (CurrentAttackRow && PartComponent)
	{
		PartComponent->TryGetTraceSegmentByAttackId(
			CurrentAttackRow->AttackId,
			AttackTraceDistance,
			EnemyActor->GetActorForwardVector(),
			Start,
			End);
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(EnemyActor);

	if (CurrentAttackRow && PartComponent)
	{
		TArray<AActor*> IgnoredPartActors;
		PartComponent->GetSpawnedPartActorsByAttackId(
			CurrentAttackRow->AttackId,
			IgnoredPartActors);

		for (AActor* IgnoredActor : IgnoredPartActors)
		{
			if (IsValid(IgnoredActor))
			{
				QueryParams.AddIgnoredActor(IgnoredActor);
			}
		}
	}

	FHitResult HitResult;

	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		NSCollisionChannels::EnemyWeaponTrace,
		FCollisionShape::MakeSphere(AttackTraceRadius),
		QueryParams);

	if (bHit && TryApplyDamageToTarget(HitResult.GetActor(), HitResult))
	{
		if (TraceWindowId != 0)
		{
			DamagedTraceWindowIds.Add(TraceWindowId);
		}
	}
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
