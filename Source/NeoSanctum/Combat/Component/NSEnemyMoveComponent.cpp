// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyMoveComponent.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyAttackComponent.h"
#include "NeoSanctum/Combat/Component/NSEnemyTargetComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSEnemyMoveComponent::UNSEnemyMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

FNSRetreatResult UNSEnemyMoveComponent::UpdateRetreat(
	AActor* TargetActor,
	bool bWasRetreating,
	bool bHasCurrentLocation,
	const FVector& CurrentLocation)
{
	FNSRetreatResult Result;

	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !IsValid(TargetActor))
	{
		ClearRetreat();
		return Result;
	}

	const float MinRange = GetMinAttackRange();

	if (MinRange <= 0.0f)
	{
		ClearRetreat();
		return Result;
	}

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	const float Distance = FVector::Dist(EnemyLocation, TargetLocation);
	const float ExitRange = MinRange + RetreatExitBuffer;

	Result.bShouldRetreat = bWasRetreating
		                        ? Distance < ExitRange
		                        : Distance < MinRange;

	Enemy->SetRetreating(Result.bShouldRetreat);

	if (!Result.bShouldRetreat)
	{
		return Result;
	}

	const bool bDestinationReached =
		FVector::DistSquared2D(EnemyLocation, CurrentLocation) <= FMath::Square(RetreatAcceptanceRadius);

	if (bWasRetreating && bHasCurrentLocation && !bDestinationReached)
	{
		Result.bHasLocation = true;
		Result.Location = CurrentLocation;
		return Result;
	}

	FVector AwayDirection = (EnemyLocation - TargetLocation).GetSafeNormal2D();

	if (AwayDirection.IsNearlyZero())
	{
		AwayDirection = -Enemy->GetActorForwardVector().GetSafeNormal2D();
	}

	const float RequiredDistance = FMath::Max(ExitRange - Distance, RetreatStepDistance);
	const FVector DesiredLocation = EnemyLocation + AwayDirection * RequiredDistance;

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FNavLocation ProjectedLocation;
	if (NavSystem && NavSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation))
	{
		Result.bHasLocation = true;
		Result.Location = ProjectedLocation.Location;
	}

	return Result;
}

void UNSEnemyMoveComponent::ClearRetreat()
{
	if (ANSEnemyCharacterBase* Enemy = GetOwnerEnemy())
	{
		Enemy->SetRetreating(false);
	}
}

void UNSEnemyMoveComponent::ApplyFacing(
	AAIController* Controller,
	AActor* TargetActor,
	AActor* AimActor,
	bool bFaceTarget)
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (bFaceTarget && IsValid(TargetActor))
	{
		if (!IsValid(AimActor))
		{
			AimActor = TargetActor;
		}

		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;

		if (Controller)
		{
			Controller->SetFocus(AimActor, EAIFocusPriority::Gameplay);
		}

		Enemy->UpdateCombatAimTarget(AimActor);
	}
	else
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;

		if (Controller)
		{
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
		}

		Enemy->ClearCombatAimTarget();
	}
}

bool UNSEnemyMoveComponent::IsWithinAttackRange(AActor* TargetActor) const
{
	const ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	const UNSEnemyData* EnemyData = GetEnemyData();

	if (!Enemy || !EnemyData || !IsValid(TargetActor))
	{
		return false;
	}

	const UNSEnemyAttackComponent* AttackComponent =
		Enemy->FindComponentByClass<UNSEnemyAttackComponent>();

	const UNSEnemyTargetComponent* TargetComponent =
		Enemy->FindComponentByClass<UNSEnemyTargetComponent>();

	if (!AttackComponent || !TargetComponent || !TargetComponent->IsValidLivingTarget(TargetActor))
	{
		return false;
	}

	bool bHasDirectLineOfSight = false;
	AActor* AttackActor = TargetComponent->ResolveAttackActor(TargetActor, bHasDirectLineOfSight);

	if (!IsValid(AttackActor))
	{
		return false;
	}

	const float Distance = FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (AttackRow &&
			AttackComponent->CanUseAttack(*AttackRow, TargetActor, AttackActor, Distance, bHasDirectLineOfSight))
		{
			return true;
		}
	}

	return false;
}

ANSEnemyCharacterBase* UNSEnemyMoveComponent::GetOwnerEnemy() const
{
	return Cast<ANSEnemyCharacterBase>(GetOwner());
}

const UNSEnemyData* UNSEnemyMoveComponent::GetEnemyData() const
{
	const ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	return Enemy ? Enemy->GetEnemyData() : nullptr;
}

float UNSEnemyMoveComponent::GetMinAttackRange() const
{
	const UNSEnemyData* EnemyData = GetEnemyData();

	if (!EnemyData)
	{
		return 0.0f;
	}

	float MinRange = TNumericLimits<float>::Max();
	bool bFoundAttack = false;

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (!AttackRow || !AttackRow->AbilityClass)
		{
			continue;
		}

		bFoundAttack = true;
		MinRange = FMath::Min(MinRange, AttackRow->Condition.MinRange);
	}

	return bFoundAttack ? MinRange : 0.0f;
}
