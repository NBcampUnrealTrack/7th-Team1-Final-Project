// Copyright 2026 One Team. All rights reserved.

#include "NSBBTypes.h"

namespace NSBB
{
	namespace Target
	{
		const FName TargetActor = TEXT("TargetActor");
		const FName AttackActor = TEXT("AttackActor");
		const FName TargetLastKnownLocation = TEXT("TargetLastKnownLocation");
		const FName HasTargetLineOfSight = TEXT("bHasTargetLineOfSight");
		const FName ActualLocation = TEXT("TargetActualLocation");
		const FName MoveResolveType = TEXT("TargetMoveResolveType");
		const FName IsAirborne = TEXT("bTargetAirborne");
	}

	namespace Combat
	{
		const FName CanAttack = TEXT("bCanAttack");
		const FName IsAttacking = TEXT("bIsAttacking");
		const FName IsHitReacting = TEXT("bIsHitReacting");
	}

	namespace Phase
	{
		const FName PhasePatternLocked = TEXT("bPhasePatternLocked");
		const FName CurrentPhaseId = TEXT("CurrentPhaseId");
	}

	namespace Movement
	{
		const FName ShouldRetreat = TEXT("bShouldRetreat");
		const FName RetreatLocation = TEXT("RetreatLocation");
		const FName PatrolLocation = TEXT("PatrolLocation");
		const FName ResolvedTargetMoveLocation = TEXT("ResolvedTargetMoveLocation");
		const FName HasResolvedTargetMoveLocation = TEXT("bHasResolvedTargetMoveLocation");
		const FName ArrivedBelowAirborneTarget = TEXT("bArrivedBelowAirborneTarget");
	}

	namespace Melee
	{
		const FName HasAttackReservation = TEXT("bHasMeleeAttackReservation");
		const FName CanApproachTarget = TEXT("bCanApproachMeleeTarget");
		const FName EQSQuery = TEXT("MeleeEQSQuery");
		const FName ApproachLocation = TEXT("MeleeApproachLocation");
		const FName EQSNeedsRefresh = TEXT("bMeleeEQSNeedsRefresh");
	}

	namespace Boss
	{
		const FName CurrentModeTag = TEXT("CurrentModeTag");
	}
}
