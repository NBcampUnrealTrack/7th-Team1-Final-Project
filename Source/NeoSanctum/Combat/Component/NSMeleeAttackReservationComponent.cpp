// Copyright 2026 One Team. All rights reserved.

#include "NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

UNSMeleeAttackReservationComponent::UNSMeleeAttackReservationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;

	SetIsReplicatedByDefault(false);
}


bool UNSMeleeAttackReservationComponent::HasReservation(ANSEnemyCharacterBase* Enemy)
{
	if (!GetWorld())
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

			return Reservation.ExpirationTime <= CurrentTime;
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
