// Copyright 2026 One Team. All rights reserved.

#include "NSBossTargetComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSEnemyThreatComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSBossTargetComponent::UNSBossTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSBossTargetComponent::ResetTargets()
{
	CurrentAttackTargets.Reset();
}

void UNSBossTargetComponent::BuildAttackTargets(
	AActor* PrimaryTarget,
	const FNSEnemyAttackRow& AttackRow)
{
	CurrentAttackTargets.Reset();

	if (AttackRow.MaxTargets <= 0)
	{
		return;
	}

	TArray<AActor*> Candidates;
	CollectCandidatesByPolicy(
		PrimaryTarget,
		AttackRow,
		Candidates);

	if (Candidates.IsEmpty())
	{
		return;
	}

	if (AttackRow.TargetPolicy == ENSBossTargetPolicy::RandomKnown)
	{
		ShuffleCandidates(Candidates);
	}
	else
	{
		SortCandidates(PrimaryTarget, Candidates);
	}

	for (AActor* Candidate : Candidates)
	{
		if (!IsValidLivingTarget(Candidate))
		{
			continue;
		}

		CurrentAttackTargets.Add(Candidate);

		if (CurrentAttackTargets.Num() >= AttackRow.MaxTargets)
		{
			break;
		}
	}
}

void UNSBossTargetComponent::GetCurrentAttackTargets(TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	for (const TWeakObjectPtr<AActor>& TargetPtr : CurrentAttackTargets)
	{
		AActor* TargetActor = TargetPtr.Get();

		if (IsValidLivingTarget(TargetActor))
		{
			OutTargets.Add(TargetActor);
		}
	}
}

AActor* UNSBossTargetComponent::GetAttackTarget(int32 Index) const
{
	if (!CurrentAttackTargets.IsValidIndex(Index))
	{
		return nullptr;
	}

	AActor* TargetActor = CurrentAttackTargets[Index].Get();
	return IsValidLivingTarget(TargetActor) ? TargetActor : nullptr;
}

AActor* UNSBossTargetComponent::GetPrimaryAttackTarget() const
{
	for (const TWeakObjectPtr<AActor>& TargetPtr : CurrentAttackTargets)
	{
		AActor* TargetActor = TargetPtr.Get();

		if (IsValidLivingTarget(TargetActor))
		{
			return TargetActor;
		}
	}

	return nullptr;
}

int32 UNSBossTargetComponent::GetAttackTargetCount() const
{
	int32 Count = 0;

	for (const TWeakObjectPtr<AActor>& TargetPtr : CurrentAttackTargets)
	{
		if (IsValidLivingTarget(TargetPtr.Get()))
		{
			++Count;
		}
	}

	return Count;
}

void UNSBossTargetComponent::SortCandidates(
	AActor* PrimaryTarget,
	TArray<AActor*>& Candidates) const
{
	const APawn* OwnerPawn = GetOwnerPawn();
	const FVector ReferenceLocation = IsValid(PrimaryTarget)
		                                  ? PrimaryTarget->GetActorLocation()
		                                  : OwnerPawn
		                                  ? OwnerPawn->GetActorLocation()
		                                  : FVector::ZeroVector;

	Candidates.Sort(
		[PrimaryTarget, ReferenceLocation](const AActor& A, const AActor& B)
		{
			if (&A == PrimaryTarget)
			{
				return true;
			}

			if (&B == PrimaryTarget)
			{
				return false;
			}

			const float DistanceASq =
				FVector::DistSquared(ReferenceLocation, A.GetActorLocation());

			const float DistanceBSq =
				FVector::DistSquared(ReferenceLocation, B.GetActorLocation());

			return DistanceASq < DistanceBSq;
		});
}

bool UNSBossTargetComponent::IsValidLivingTarget(const AActor* Target) const
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

APawn* UNSBossTargetComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

UNSEnemyThreatComponent* UNSBossTargetComponent::GetThreatComponent() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor
		       ? OwnerActor->FindComponentByClass<UNSEnemyThreatComponent>()
		       : nullptr;
}


void UNSBossTargetComponent::CollectCandidatesByPolicy(
	AActor* PrimaryTarget,
	const FNSEnemyAttackRow& AttackRow,
	TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	switch (AttackRow.TargetPolicy)
	{
	case ENSBossTargetPolicy::PrimaryOnly:
		CollectPrimaryOnlyTargets(PrimaryTarget, OutCandidates);
		break;

	case ENSBossTargetPolicy::NearbyKnown:
		CollectNearbyKnownTargets(PrimaryTarget, AttackRow, OutCandidates);
		break;

	case ENSBossTargetPolicy::AllKnown:
		CollectAllKnownTargets(PrimaryTarget, AttackRow, OutCandidates);
		break;

	case ENSBossTargetPolicy::RandomKnown:
		CollectAllKnownTargets(PrimaryTarget, AttackRow, OutCandidates);
		break;

	default:
		CollectPrimaryOnlyTargets(PrimaryTarget, OutCandidates);
		break;
	}
}

void UNSBossTargetComponent::CollectPrimaryOnlyTargets(
	AActor* PrimaryTarget,
	TArray<AActor*>& OutCandidates) const
{
	if (IsValidLivingTarget(PrimaryTarget))
	{
		OutCandidates.AddUnique(PrimaryTarget);
	}
}

void UNSBossTargetComponent::CollectNearbyKnownTargets(
	AActor* PrimaryTarget,
	const FNSEnemyAttackRow& AttackRow,
	TArray<AActor*>& OutCandidates) const
{
	const APawn* OwnerPawn = GetOwnerPawn();

	const FVector ReferenceLocation =
		IsValid(PrimaryTarget)
			? PrimaryTarget->GetActorLocation()
			: OwnerPawn
			? OwnerPawn->GetActorLocation()
			: FVector::ZeroVector;

	auto TryAddCandidate =
		[this, &OutCandidates, PrimaryTarget, ReferenceLocation, SearchRadius = AttackRow.TargetSearchRadius,
			bIncludePrimaryTarget = AttackRow.bIncludePrimaryTarget](AActor* Candidate)
	{
		if (!IsValidLivingTarget(Candidate))
		{
			return;
		}

		if (Candidate == PrimaryTarget && !bIncludePrimaryTarget)
		{
			return;
		}

		if (SearchRadius > 0.0f)
		{
			const float DistanceSq =
				FVector::DistSquared(ReferenceLocation, Candidate->GetActorLocation());

			if (DistanceSq > FMath::Square(SearchRadius))
			{
				return;
			}
		}

		OutCandidates.AddUnique(Candidate);
	};

	if (AttackRow.bIncludePrimaryTarget)
	{
		TryAddCandidate(PrimaryTarget);
	}

	if (const UNSEnemyThreatComponent* ThreatComponent = GetThreatComponent())
	{
		TArray<AActor*> KnownTargets;
		ThreatComponent->GetKnownTargets(KnownTargets);

		for (AActor* KnownTarget : KnownTargets)
		{
			TryAddCandidate(KnownTarget);
		}
	}
}

void UNSBossTargetComponent::CollectAllKnownTargets(
	AActor* PrimaryTarget,
	const FNSEnemyAttackRow& AttackRow,
	TArray<AActor*>& OutCandidates) const
{
	if (AttackRow.bIncludePrimaryTarget && IsValidLivingTarget(PrimaryTarget))
	{
		OutCandidates.AddUnique(PrimaryTarget);
	}

	if (const UNSEnemyThreatComponent* ThreatComponent = GetThreatComponent())
	{
		TArray<AActor*> KnownTargets;
		ThreatComponent->GetKnownTargets(KnownTargets);

		for (AActor* KnownTarget : KnownTargets)
		{
			if (KnownTarget == PrimaryTarget && !AttackRow.bIncludePrimaryTarget)
			{
				continue;
			}

			if (IsValidLivingTarget(KnownTarget))
			{
				OutCandidates.AddUnique(KnownTarget);
			}
		}
	}
}

void UNSBossTargetComponent::ShuffleCandidates(TArray<AActor*>& Candidates) const
{
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Candidates.Swap(Index, SwapIndex);
	}
}
