// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyMoveComponent.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Abilities/GameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
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

	if (Enemy->IsTraversingNavLink())
	{
		if (Controller)
		{
			Controller->ClearFocus(EAIFocusPriority::Gameplay);
		}

		Enemy->ClearCombatAimTarget();
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

	if (!Enemy || Enemy->IsTraversingNavLink() || !EnemyData || !IsValid(TargetActor))
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

void UNSEnemyMoveComponent::UpdateNavigationRecovery(
	AAIController* Controller, 
	float DeltaTime)
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	if (!Controller || !Enemy || !Enemy->HasAuthority() || Enemy->IsDead() || Enemy->IsInPool() ||
		Enemy->IsHitReacting() || Enemy->IsTraversingNavLink())
	{
		ResetNavigationRecovery();
		return;
	}

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Movement || !NavSystem || Movement->IsFalling())
	{
		ResetNavigationRecovery();
		return;
	}

	const FVector CurrentLocation = Enemy->GetActorLocation();

	FNavLocation ProjectedLocation;
	const bool bProjected = NavSystem->ProjectPointToNavigation(
		CurrentLocation,
		ProjectedLocation,
		NavProjectionExtent);

	const bool bOnNavMesh =
		bProjected &&
		FVector::DistSquared2D(CurrentLocation, ProjectedLocation.Location) <= FMath::Square(NavOutsideTolerance);

	if (bOnNavMesh)
	{
		LastValidNavLocation = ProjectedLocation.Location;
		bHasLastValidNavLocation = true;
	}

	if (!bHasLastObservedLocation)
	{
		LastObservedLocation = CurrentLocation;
		bHasLastObservedLocation = true;
		return;
	}

	const bool bMovedEnough =
		FVector::DistSquared2D(CurrentLocation, LastObservedLocation) > FMath::Square(StuckMoveTolerance);

	const bool bNearlyStopped =
		Movement->Velocity.SizeSquared2D() <= FMath::Square(StuckVelocityTolerance);

	const bool bExpectedToMove =
		Controller->GetMoveStatus() == EPathFollowingStatus::Moving;

	if (!bOnNavMesh || (bExpectedToMove && bNearlyStopped && !bMovedEnough))
	{
		NavRecoveryElapsed += DeltaTime;
	}
	else
	{
		NavRecoveryElapsed = 0.0f;
		LastObservedLocation = CurrentLocation;
		return;
	}

	if (bMovedEnough)
	{
		LastObservedLocation = CurrentLocation;
	}

	if (NavRecoveryElapsed < NavRecoveryDelay)
	{
		return;
	}

	const FVector SearchOrigin =
		bHasLastValidNavLocation ? LastValidNavLocation : bProjected ? ProjectedLocation.Location : CurrentLocation;

	FNavLocation RecoveryLocation;
	bool bFoundRecoveryLocation =
		NavSystem->GetRandomReachablePointInRadius(SearchOrigin, RecoverySearchRadius, RecoveryLocation);

	if (!bFoundRecoveryLocation && bProjected)
	{
		RecoveryLocation = ProjectedLocation;
		bFoundRecoveryLocation = true;
	}

	if (!bFoundRecoveryLocation)
	{
		FNavLocation WideProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(
			CurrentLocation,
			WideProjectedLocation,
			FVector(2000.0f, 2000.0f, 1000.0f)))
		{
			bFoundRecoveryLocation =
				NavSystem->GetRandomReachablePointInRadius(
					WideProjectedLocation.Location,
					RecoverySearchRadius,
					RecoveryLocation);

			if (!bFoundRecoveryLocation)
			{
				RecoveryLocation = WideProjectedLocation;
				bFoundRecoveryLocation = true;
			}
		}
	}

	if (bFoundRecoveryLocation)
	{
		const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;

		Controller->StopMovement();
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);

		Enemy->SetActorLocation(
			RecoveryLocation.Location + FVector(0.0f, 0.0f, HalfHeight),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	ResetNavigationRecovery();
}

void UNSEnemyMoveComponent::ResetNavigationRecovery()
{
	NavRecoveryElapsed = 0.0f;
	bHasLastObservedLocation = false;
	LastObservedLocation = FVector::ZeroVector;
	bHasLastValidNavLocation = false;
	LastValidNavLocation = FVector::ZeroVector;
}

FNSResolvedTargetMoveResult UNSEnemyMoveComponent::ResolveTargetMoveLocation(
	AActor* TargetActor,
	AAIController* Controller)
{
	FNSResolvedTargetMoveResult Result;

	if (!bEnableTargetMoveResolve || !IsValid(TargetActor))
	{
		return Result;
	}

	Result.ActualLocation = TargetActor->GetActorLocation();
	Result.bTargetAirborne = IsTargetAirborne(TargetActor);

	auto UpdateArrivedBelowState = [this, &Result]()
	{
		const ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
		if (!Enemy || !Result.bHasMoveLocation)
		{
			Result.bArrivedBelowAirborneTarget = false;
			return;
		}

		const FVector EnemyLocation = Enemy->GetActorLocation();

		const bool bArrivedIn2D =
			FVector::DistSquared2D(EnemyLocation, Result.MoveLocation) <=
			FMath::Square(ArriveBelowDistance);

		const bool bHasEnoughHeightDifference =
			FMath::Abs(Result.ActualLocation.Z - EnemyLocation.Z) >=
			MinimumAirborneHeight;

		Result.bArrivedBelowAirborneTarget =
			Result.bTargetAirborne &&
			bArrivedIn2D &&
			bHasEnoughHeightDifference;
	};

	auto AcceptLocation =
		[this, Controller, &Result, &UpdateArrivedBelowState](
		const FVector& Location,
		ENSTargetMoveResolveType ResolveType)
	{
		if (!IsReachableMoveLocation(Controller, Location))
		{
			return false;
		}

		Result.MoveLocation = Location;
		Result.ResolveType = ResolveType;
		Result.bHasMoveLocation = true;

		MarkReachableLocation(Location);
		UpdateArrivedBelowState();

		return true;
	};

	FVector ProjectedLocation = FVector::ZeroVector;

	if (!Result.bTargetAirborne &&
		ProjectToNavigation(Result.ActualLocation, ProjectedLocation) &&
		AcceptLocation(ProjectedLocation, ENSTargetMoveResolveType::Actual))
	{
		return Result;
	}

	FVector GroundLocation = FVector::ZeroVector;
	if (TraceGroundLocation(TargetActor, Result.ActualLocation, GroundLocation) &&
		ProjectToNavigation(GroundLocation, ProjectedLocation) &&
		AcceptLocation(ProjectedLocation, ENSTargetMoveResolveType::GroundProjected))
	{
		return Result;
	}

	FVector NearbyLocation = FVector::ZeroVector;
	if (FindNearbyReachableLocation(
			TargetActor,
			Result.ActualLocation,
			Controller,
			NearbyLocation) &&
		AcceptLocation(NearbyLocation, ENSTargetMoveResolveType::NearbyProjected))
	{
		return Result;
	}

	if (bHasLastReachableMoveLocation)
	{
		Result.MoveLocation = LastReachableMoveLocation;
		Result.ResolveType = ENSTargetMoveResolveType::LastReachable;
		Result.bHasMoveLocation = true;

		UpdateArrivedBelowState();

		return Result;
	}

	Result.ResolveType = ENSTargetMoveResolveType::Invalid;
	Result.bHasMoveLocation = false;
	Result.bArrivedBelowAirborneTarget = false;

	return Result;
}

void UNSEnemyMoveComponent::ResetTargetMoveResolveState()
{
	bHasLastReachableMoveLocation = false;
	LastReachableMoveLocation = FVector::ZeroVector;

	bHasReachabilityCache = false;
	CachedReachabilityLocation = FVector::ZeroVector;
	bCachedReachabilityResult = false;
	LastReachabilityCheckTime = -1000.0;
}

bool UNSEnemyMoveComponent::IsTargetAirborne(const AActor* TargetActor) const
{
	const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent =
		TargetCharacter->GetCharacterMovement();

	return MovementComponent && MovementComponent->IsFalling();
}

bool UNSEnemyMoveComponent::TraceGroundLocation(
	const AActor* TargetActor,
	const FVector& SourceLocation,
	FVector& OutGroundLocation) const
{
	const UWorld* World = GetWorld();
	if (!World || TargetGroundTraceDistance <= 0.0f)
	{
		return false;
	}

	const FVector TraceStart = SourceLocation;
	const FVector TraceEnd =
		SourceLocation - FVector::UpVector * TargetGroundTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSTargetGroundTrace), false, GetOwner());
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(TargetActor);

	FHitResult HitResult;

	bool bHit = false;

	if (TargetGroundSweepRadius > KINDA_SMALL_NUMBER)
	{
		bHit = World->SweepSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			TargetGroundTraceChannel.GetValue(),
			FCollisionShape::MakeSphere(TargetGroundSweepRadius),
			QueryParams);
	}
	else
	{
		bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			TargetGroundTraceChannel.GetValue(),
			QueryParams);
	}

	if (!bHit)
	{
		return false;
	}

	OutGroundLocation = HitResult.ImpactPoint;
	return true;
}

bool UNSEnemyMoveComponent::ProjectToNavigation(
	const FVector& SourceLocation,
	FVector& OutNavLocation) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UNavigationSystemV1* NavSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	if (!NavSystem)
	{
		return false;
	}

	FNavLocation ProjectedLocation;

	const bool bProjected = NavSystem->ProjectPointToNavigation(
		SourceLocation,
		ProjectedLocation,
		TargetNavProjectionExtent);

	if (!bProjected)
	{
		return false;
	}

	OutNavLocation = ProjectedLocation.Location;
	return true;
}

bool UNSEnemyMoveComponent::FindNearbyReachableLocation(
	const AActor* TargetActor,
	const FVector& TargetLocation,
	AAIController* Controller,
	FVector& OutLocation)
{
	if (NearbySampleCount <= 0 || NearbySearchRadius <= 0.0f)
	{
		return false;
	}

	double BestDistanceSq = TNumericLimits<double>::Max();
	bool bFoundLocation = false;

	for (int32 Index = 0; Index < NearbySampleCount; ++Index)
	{
		const double Angle =
			2.0 * UE_PI * static_cast<double>(Index) /
			static_cast<double>(NearbySampleCount);

		const FVector Direction(
			FMath::Cos(Angle),
			FMath::Sin(Angle),
			0.0);

		const FVector CandidateLocation =
			TargetLocation + Direction * NearbySearchRadius;

		FVector GroundLocation = FVector::ZeroVector;
		if (!TraceGroundLocation(TargetActor, CandidateLocation, GroundLocation))
		{
			continue;
		}

		FVector NavLocation = FVector::ZeroVector;
		if (!ProjectToNavigation(GroundLocation, NavLocation))
		{
			continue;
		}

		if (!IsReachableMoveLocation(Controller, NavLocation))
		{
			continue;
		}

		const double DistanceSq =
			FVector::DistSquared2D(TargetLocation, NavLocation);

		if (DistanceSq >= BestDistanceSq)
		{
			continue;
		}

		BestDistanceSq = DistanceSq;
		OutLocation = NavLocation;
		bFoundLocation = true;
	}

	return bFoundLocation;
}

bool UNSEnemyMoveComponent::IsReachableMoveLocation(
	AAIController* Controller,
	const FVector& CandidateLocation)
{
	if (!bCheckTargetMoveReachability)
	{
		return true;
	}

	if (!Controller)
	{
		return false;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const double CurrentTime = World->GetTimeSeconds();

	const bool bCanUseCachedReachability =
		bHasReachabilityCache &&
		CurrentTime - LastReachabilityCheckTime < ReachabilityCheckInterval &&
		FVector::DistSquared2D(CachedReachabilityLocation, CandidateLocation) <=
		FMath::Square(50.0f);

	if (bCanUseCachedReachability)
	{
		return bCachedReachabilityResult;
	}

	UNavigationPath* Path =
		UNavigationSystemV1::FindPathToLocationSynchronously(
			World,
			Pawn->GetActorLocation(),
			CandidateLocation,
			Pawn);

	const bool bReachable =
		IsValid(Path) &&
		Path->IsValid() &&
		!Path->IsPartial();

	bHasReachabilityCache = true;
	CachedReachabilityLocation = CandidateLocation;
	bCachedReachabilityResult = bReachable;
	LastReachabilityCheckTime = CurrentTime;

	return bReachable;
}

void UNSEnemyMoveComponent::MarkReachableLocation(const FVector& Location)
{
	LastReachableMoveLocation = Location;
	bHasLastReachableMoveLocation = true;
}
