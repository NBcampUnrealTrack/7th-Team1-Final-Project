// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyTargetComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NeoSanctum/Combat/Component/NSEnemyPartComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Interaction/Prop/NSDestructibleObjectBase.h"

UNSEnemyTargetComponent::UNSEnemyTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

AActor* UNSEnemyTargetComponent::ResolveAttackActor(
	AActor* TargetActor,
	bool& bOutHasDirectLineOfSight
) const
{
	bOutHasDirectLineOfSight = false;

	if (!IsValidLivingTarget(TargetActor) || !GetWorld())
	{
		return nullptr;
	}

	const APawn* OwnerPawn = GetOwnerPawn();
	if (!OwnerPawn)
	{
		return nullptr;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyCoverAttackTrace), false, OwnerPawn);
	QueryParams.AddIgnoredActor(OwnerPawn);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		GetTraceStart(),
		GetAimLocation(TargetActor),
		NSCollisionChannels::CombatSight,
		QueryParams
	);

	if (!bHit || HitResult.GetActor() == TargetActor)
	{
		bOutHasDirectLineOfSight = true;
		return TargetActor;
	}

	if (!bAttackDestructibleCover)
	{
		return nullptr;
	}

	AActor* HitActor = HitResult.GetActor();
	if (IsValid(HitActor) && HitActor->IsA<ANSDestructibleObjectBase>())
	{
		return HitActor;
	}

	return nullptr;
}

bool UNSEnemyTargetComponent::IsValidLivingTarget(const AActor* Target) const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	const UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	for (UAttributeSet* AttributeSet : ASC->GetSpawnedAttributes())
	{
		if (!AttributeSet)
		{
			continue;
		}

		if (FProperty* Prop = AttributeSet->GetClass()->FindPropertyByName(TEXT("Health")))
		{
			const FGameplayAttribute HealthAttribute(Prop);
			return ASC->GetNumericAttribute(HealthAttribute) > 0.0f;
		}
	}

	return false;
}

APawn* UNSEnemyTargetComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

FVector UNSEnemyTargetComponent::GetAimLocation(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return FVector::ZeroVector;
	}

	FVector Origin = Actor->GetActorLocation();
	FVector Extent = FVector::ZeroVector;

	if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		Origin = Primitive->Bounds.Origin;
		Extent = Primitive->Bounds.BoxExtent;
	}

	return Origin + FVector::UpVector * (Extent.Z * CoverAimZRatio);
}

FVector UNSEnemyTargetComponent::GetTraceStart() const
{
	const APawn* OwnerPawn = GetOwnerPawn();
	if (!OwnerPawn)
	{
		return FVector::ZeroVector;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerPawn);
	const UNSEnemyPartComponent* PartComponent =
		OwnerPawn->FindComponentByClass<UNSEnemyPartComponent>();

	if (PartComponent)
	{
		FTransform MuzzleTransform;

		const FNSEnemyAttackRow* CurrentAttackRow = EnemyAgent ? EnemyAgent->GetCurrentAttackRow() : nullptr;

		if (CurrentAttackRow &&
			PartComponent->TryGetMuzzleTransformByAttackId(
				CurrentAttackRow->AttackId,
				MuzzleTransform))
		{
			return MuzzleTransform.GetLocation();
		}

		if (PartComponent->TryGetAnyMuzzleTransform(MuzzleTransform))
		{
			return MuzzleTransform.GetLocation();
		}
	}

	if (EnemyAgent)
	{
		return EnemyAgent->GetAimLocation();
	}

	return OwnerPawn->GetPawnViewLocation();
}
