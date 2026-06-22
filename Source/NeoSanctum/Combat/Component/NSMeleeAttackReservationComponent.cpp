// Copyright 2026 One Team. All rights reserved.

#include "NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

UNSMeleeAttackReservationComponent::UNSMeleeAttackReservationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;

	SetIsReplicatedByDefault(false);
}

void UNSMeleeAttackReservationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
	{
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();

	CleanupInvalidEntries(CurrentTime);
	PromoteQueuedRequests(CurrentTime);
}

ENSMeleeReservationRequestResult UNSMeleeAttackReservationComponent::RequestReservation(
	ANSEnemyCharacterBase* Enemy,
	double LastDamagedTime)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || !IsEnemyValid(Enemy))
	{
		return ENSMeleeReservationRequestResult::Rejected;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();

	CleanupInvalidEntries(CurrentTime);

	for (const FActiveReservation& Reservation : ActiveReservations)
	{
		if (Reservation.Enemy == Enemy)
		{
			return ENSMeleeReservationRequestResult::Reserved;
		}
	}

	FQueuedRequest* ExistingRequest = QueuedRequests.FindByPredicate(
		[Enemy](const FQueuedRequest& Request)
		{
			return Request.Enemy == Enemy;
		});

	if (ExistingRequest)
	{
		ExistingRequest->LastDamagedTime = FMath::Max(
			ExistingRequest->LastDamagedTime,
			LastDamagedTime);
	}
	else
	{
		FQueuedRequest& Request = QueuedRequests.AddDefaulted_GetRef();

		Request.Enemy = Enemy;
		Request.RequestTime = CurrentTime;
		Request.LastDamagedTime = LastDamagedTime;
	}

	PromoteQueuedRequests(CurrentTime);

	const bool bReserved = ActiveReservations.ContainsByPredicate(
		[Enemy](const FActiveReservation& Reservation)
		{
			return Reservation.Enemy == Enemy;
		});

	return bReserved
		       ? ENSMeleeReservationRequestResult::Reserved
		       : ENSMeleeReservationRequestResult::Queued;
}

bool UNSMeleeAttackReservationComponent::HasReservation(ANSEnemyCharacterBase* Enemy)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || !IsEnemyValid(Enemy))
	{
		return false;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();

	CleanupInvalidEntries(CurrentTime);

	return ActiveReservations.ContainsByPredicate(
		[Enemy](const FActiveReservation& Reservation)
		{
			return Reservation.Enemy == Enemy;
		});
}

void UNSMeleeAttackReservationComponent::MarkAttackStarted(ANSEnemyCharacterBase* Enemy)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || !IsEnemyValid(Enemy))
	{
		return;
	}

	FActiveReservation* Reservation = ActiveReservations.FindByPredicate(
		[Enemy](const FActiveReservation& ActiveReservation)
		{
			return ActiveReservation.Enemy == Enemy;
		});

	if (!Reservation)
	{
		return;
	}

	Reservation->Phase = ENSMeleeReservationPhase::Attacking;

	Reservation->ExpirationTime = GetWorld()->GetTimeSeconds() + AttackReservationSafetyTimeout;
}

void UNSMeleeAttackReservationComponent::ReleaseReservation(ANSEnemyCharacterBase* Enemy, bool bStartReacquireCooldown)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Enemy || !GetWorld())
	{
		return;
	}
	
	const double CurrentTime = GetWorld()->GetTimeSeconds();

	const int32 RemovedActiveCount = ActiveReservations.RemoveAll(
		[Enemy](const FActiveReservation& Reservation)
		{
			return Reservation.Enemy == Enemy;
		});

	QueuedRequests.RemoveAll(
		[Enemy](const FQueuedRequest& Request)
		{
			return Request.Enemy == Enemy;
		});

	if (bStartReacquireCooldown && RemovedActiveCount > 0)
	{
		StartReacquireCooldown(Enemy, CurrentTime);
	}

	PromoteQueuedRequests(GetWorld()->GetTimeSeconds());
}

void UNSMeleeAttackReservationComponent::SetMaxConcurrentAttackers(int32 InMaxAttackers)
{
	MaxConcurrentAttackers = FMath::Max(InMaxAttackers, 1);
}

void UNSMeleeAttackReservationComponent::CleanupInvalidEntries(double CurrentTime)
{
	ActiveReservations.RemoveAll(
		[this, CurrentTime](const FActiveReservation& Reservation)
		{
			ANSEnemyCharacterBase* Enemy = Reservation.Enemy.Get();

			if (!IsEnemyValid(Enemy))
			{
				return true;
			}

			if (Reservation.ExpirationTime > CurrentTime)
			{
				return false;
			}
			
			StartReacquireCooldown(Enemy, CurrentTime);
			return true;
		});

	QueuedRequests.RemoveAll(
		[this](const FQueuedRequest& Request)
		{
			return !IsEnemyValid(Request.Enemy.Get());
		});

	for (auto It = ReacquireBlockedUntil.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || It.Value() <= CurrentTime)
		{
			It.RemoveCurrent();
		}
	}
}

void UNSMeleeAttackReservationComponent::PromoteQueuedRequests(double CurrentTime)
{
	while (ActiveReservations.Num() < MaxConcurrentAttackers)
	{
		const int32 BestRequestIndex = FindBestQueuedRequestIndex(CurrentTime);

		if (!QueuedRequests.IsValidIndex(BestRequestIndex))
		{
			return;
		}

		const FQueuedRequest Request = QueuedRequests[BestRequestIndex];

		QueuedRequests.RemoveAt(BestRequestIndex);

		ANSEnemyCharacterBase* Enemy = Request.Enemy.Get();

		if (!IsEnemyValid(Enemy))
		{
			continue;
		}

		FActiveReservation& Reservation = ActiveReservations.AddDefaulted_GetRef();

		Reservation.Enemy = Enemy;
		Reservation.Phase = ENSMeleeReservationPhase::Approaching;

		Reservation.ExpirationTime = CurrentTime + ApproachReservationDuration;
	}
}

bool UNSMeleeAttackReservationComponent::IsEnemyValid(const ANSEnemyCharacterBase* Enemy) const
{
	return IsValid(Enemy) && !Enemy->IsDead() && !Enemy->IsInPool();
}

bool UNSMeleeAttackReservationComponent::IsReacquireBlocked(
	ANSEnemyCharacterBase* Enemy,
	double CurrentTime) const
{
	const double* BlockedUntil = ReacquireBlockedUntil.Find(Enemy);

	return BlockedUntil && *BlockedUntil > CurrentTime;
}

int32 UNSMeleeAttackReservationComponent::FindBestQueuedRequestIndex(double CurrentTime) const
{
	int32 BestIndex = INDEX_NONE;

	for (int32 Index = 0; Index < QueuedRequests.Num(); ++Index)
	{
		const FQueuedRequest& Candidate = QueuedRequests[Index];

		ANSEnemyCharacterBase* CandidateEnemy = Candidate.Enemy.Get();

		if (!IsEnemyValid(CandidateEnemy) || IsReacquireBlocked(CandidateEnemy, CurrentTime))
		{
			continue;
		}

		if (BestIndex == INDEX_NONE)
		{
			BestIndex = Index;
			continue;
		}

		const FQueuedRequest& Best = QueuedRequests[BestIndex];

		const bool bCandidateRecentlyDamaged =
			Candidate.LastDamagedTime >= 0.0 &&
			CurrentTime -
			Candidate.LastDamagedTime <= RecentDamagePriorityDuration;

		const bool bBestRecentlyDamaged =
			Best.LastDamagedTime >= 0.0 &&
			CurrentTime -
			Best.LastDamagedTime <= RecentDamagePriorityDuration;

		if (bCandidateRecentlyDamaged != bBestRecentlyDamaged)
		{
			if (bCandidateRecentlyDamaged)
			{
				BestIndex = Index;
			}

			continue;
		}

		if (Candidate.RequestTime < Best.RequestTime)
		{
			BestIndex = Index;
		}
	}

	return BestIndex;
}

void UNSMeleeAttackReservationComponent::StartReacquireCooldown(ANSEnemyCharacterBase* Enemy, double CurrentTime)
{
	if (!IsEnemyValid(Enemy))
	{
		return;
	}

	const float MinCooldown = FMath::Min(ReacquireCooldownMin, ReacquireCooldownMax);
	const float MaxCooldown = FMath::Max(ReacquireCooldownMin, ReacquireCooldownMax);

	const double NewBlockedUntil = CurrentTime + FMath::FRandRange(MinCooldown, MaxCooldown);

	double& BlockedUntil = ReacquireBlockedUntil.FindOrAdd(Enemy);
	BlockedUntil = FMath::Max(BlockedUntil, NewBlockedUntil);
}

void UNSMeleeAttackReservationComponent::MarkAttackInterrupted(ANSEnemyCharacterBase* Enemy)
{
	if (!GetOwner() ||
		!GetOwner()->HasAuthority() ||
		!GetWorld() ||
		!IsEnemyValid(Enemy))
	{
		return;
	}

	FActiveReservation* Reservation = ActiveReservations.FindByPredicate(
		[Enemy](const FActiveReservation& ActiveReservation)
		{
			return ActiveReservation.Enemy == Enemy;
		});

	if (!Reservation)
	{
		return;
	}

	Reservation->Phase = ENSMeleeReservationPhase::Approaching;

	Reservation->ExpirationTime = GetWorld()->GetTimeSeconds() + ApproachReservationDuration;
}
