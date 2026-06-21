// Copyright 2026 One Team. All rights reserved.

#include "NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

UNSMeleeAttackReservationComponent::UNSMeleeAttackReservationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;

	SetIsReplicatedByDefault(false);
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
