// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyThreatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

UNSEnemyThreatComponent::UNSEnemyThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

bool UNSEnemyThreatComponent::CanEvaluateTarget() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() >= NextTargetEvalTime;
}

void UNSEnemyThreatComponent::ScheduleNextEvaluation()
{
	if (const UWorld* World = GetWorld())
	{
		NextTargetEvalTime = World->GetTimeSeconds() + TargetEvalInterval;
	}
}

void UNSEnemyThreatComponent::ResetThreatState()
{
	ThreatRecords.Reset();
	ReacquireBlockedUntil.Reset();
	CurrentTarget.Reset();

	CurrentTargetSelectedTime = 0.0;
	LastTargetSwitchTime = 0.0;
	LastCombatProgressTime = 0.0;
	NextTargetEvalTime = 0.0;

	bAttackStartedOnCurrentTarget = false;
}

void UNSEnemyThreatComponent::NotifyAttackStarted()
{
	if (!CurrentTarget.IsValid() || !GetWorld())
	{
		return;
	}

	bAttackStartedOnCurrentTarget = true;
	LastCombatProgressTime = GetWorld()->GetTimeSeconds();
}

void UNSEnemyThreatComponent::UpdateThreatFromStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	UWorld* World = GetWorld();
	if (!World || !Actor)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	FNSEnemyThreatRecord& Record = ThreatRecords.FindOrAdd(Actor);
	Record.TargetActor = Actor;

	const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
	const FAISenseID DamageID = UAISense::GetSenseID<UAISense_Damage>();

	if (Stimulus.Type == SightID)
	{
		Record.bCurrentlyVisible = Stimulus.WasSuccessfullySensed();

		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastSeenTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();
		}
	}
	else if (Stimulus.Type == HearingID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastStimulusTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();
		}
	}
	else if (Stimulus.Type == DamageID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Record.LastStimulusTime = CurrentTime;
			Record.LastKnownLocation = Actor->GetActorLocation();

			FNSEnemyThreatDamage& DamageSample = Record.DamageSamples.AddDefaulted_GetRef();
			DamageSample.Timestamp = CurrentTime;
			DamageSample.Damage = FMath::Max(Stimulus.Strength, 0.0f);
		}
	}
}

FNSEnemyThreatUpdateResult UNSEnemyThreatComponent::UpdateTarget(
	bool bIsAttacking,
	bool bCanMaintainCurrentTarget
)
{
	FNSEnemyThreatUpdateResult Result;
	Result.PreviousTarget = CurrentTarget;

	UWorld* World = GetWorld();
	if (!World)
	{
		Result.CurrentTarget = CurrentTarget;
		Result.bTargetChanged = Result.PreviousTarget.Get() != Result.CurrentTarget.Get();
		return Result;
	}

	const double CurrentTime = World->GetTimeSeconds();

	PruneThreatRecords(CurrentTime, bCanMaintainCurrentTarget);

	AActor* CurrentTargetActor = CurrentTarget.Get();

	if (CurrentTargetActor && !IsValidLivingTarget(CurrentTargetActor))
	{
		ClearCurrentTarget(false);
		CurrentTargetActor = nullptr;
	}

	if (CurrentTargetActor && bCanMaintainCurrentTarget)
	{
		LastCombatProgressTime = CurrentTime;
	}

	if (CurrentTargetActor && bIsAttacking)
	{
		Result.CurrentTarget = CurrentTarget;
		Result.bTargetChanged = Result.PreviousTarget.Get() != Result.CurrentTarget.Get();
		return Result;
	}

	if (CurrentTargetActor &&
		!bCanMaintainCurrentTarget &&
		CurrentTime - LastCombatProgressTime >= MaxPursuitWithoutAttackDuration)
	{
		ClearCurrentTarget(true);
		CurrentTargetActor = nullptr;
	}

	if (CurrentTargetActor &&
		!bCanMaintainCurrentTarget &&
		!ThreatRecords.Contains(CurrentTargetActor))
	{
		ClearCurrentTarget(false);
		CurrentTargetActor = nullptr;
	}

	AActor* BestTarget = FindBestTarget(CurrentTime);

	if (!CurrentTargetActor)
	{
		if (BestTarget)
		{
			SetCurrentTarget(BestTarget);
		}
	}
	else if (BestTarget && ShouldSwitchTarget(BestTarget, CurrentTime))
	{
		SetCurrentTarget(BestTarget);
	}

	Result.CurrentTarget = CurrentTarget;
	Result.bTargetChanged = Result.PreviousTarget.Get() != Result.CurrentTarget.Get();

	return Result;
}

void UNSEnemyThreatComponent::RemoveTarget(AActor* TargetActor, bool bClearIfCurrent)
{
	if (!TargetActor)
	{
		return;
	}

	ThreatRecords.Remove(TargetActor);
	ReacquireBlockedUntil.Remove(TargetActor);

	if (bClearIfCurrent && CurrentTarget.Get() == TargetActor)
	{
		ClearCurrentTarget(false);
	}
}

AActor* UNSEnemyThreatComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

void UNSEnemyThreatComponent::GetKnownTargets(
	TArray<AActor*>& OutTargets,
	bool bOnlyVisible) const
{
	OutTargets.Reset();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	for (const auto& Pair : ThreatRecords)
	{
		const FNSEnemyThreatRecord& Record = Pair.Value;
		AActor* TargetActor = Record.TargetActor.Get();

		if (!IsValidLivingTarget(TargetActor))
		{
			continue;
		}

		if (!IsThreatRecordRelevant(Record, CurrentTime))
		{
			continue;
		}

		if (bOnlyVisible && !Record.bCurrentlyVisible)
		{
			continue;
		}

		OutTargets.AddUnique(TargetActor);
	}
}

void UNSEnemyThreatComponent::ClearCurrentTarget(bool bBlockReacquisition)
{
	AActor* PreviousTarget = CurrentTarget.Get();

	if (bBlockReacquisition && PreviousTarget && GetWorld())
	{
		ReacquireBlockedUntil.FindOrAdd(PreviousTarget) =
			GetWorld()->GetTimeSeconds() + TargetReacquireCooldown;
	}

	CurrentTarget.Reset();
	bAttackStartedOnCurrentTarget = false;
}

bool UNSEnemyThreatComponent::TryGetLastKnownLocation(
	const AActor* TargetActor,
	FVector& OutLocation
) const
{
	if (!TargetActor)
	{
		return false;
	}

	const FNSEnemyThreatRecord* Record = ThreatRecords.Find(TargetActor);
	if (!Record)
	{
		return false;
	}

	OutLocation = Record->LastKnownLocation;
	return true;
}

double UNSEnemyThreatComponent::GetLatestDamageTime(AActor* TargetActor) const
{
	const FNSEnemyThreatRecord* Record = ThreatRecords.Find(TargetActor);
	if (!Record)
	{
		return -1.0;
	}

	double LatestTime = -1.0;

	for (const FNSEnemyThreatDamage& Sample : Record->DamageSamples)
	{
		LatestTime = FMath::Max(LatestTime, Sample.Timestamp);
	}

	return LatestTime;
}

void UNSEnemyThreatComponent::PruneThreatRecords(
	double CurrentTime,
	bool bCanMaintainCurrentTarget
)
{
	for (auto It = ThreatRecords.CreateIterator(); It; ++It)
	{
		FNSEnemyThreatRecord& Record = It.Value();
		AActor* TargetActor = Record.TargetActor.Get();

		if (!IsValidLivingTarget(TargetActor))
		{
			It.RemoveCurrent();
			continue;
		}

		const double DamageCutoff = CurrentTime - DamageThreatWindow;

		Record.DamageSamples.RemoveAll(
			[DamageCutoff](const FNSEnemyThreatDamage& Sample)
			{
				return Sample.Timestamp < DamageCutoff;
			});

		if (!IsThreatRecordRelevant(Record, CurrentTime))
		{
			if (TargetActor == CurrentTarget.Get() && bCanMaintainCurrentTarget)
			{
				Record.LastKnownLocation = TargetActor->GetActorLocation();
				continue;
			}

			It.RemoveCurrent();
		}
	}

	for (auto It = ReacquireBlockedUntil.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || It.Value() <= CurrentTime)
		{
			It.RemoveCurrent();
		}
	}
}

AActor* UNSEnemyThreatComponent::FindBestTarget(double CurrentTime) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	AActor* BestDamageTarget = nullptr;
	float BestDamageThreat = 0.0f;
	float BestDamageTargetDistanceSq = TNumericLimits<float>::Max();

	AActor* NearestTarget = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();

	for (const auto& Pair : ThreatRecords)
	{
		AActor* TargetActor = Pair.Key.Get();
		const FNSEnemyThreatRecord& Record = Pair.Value;

		if (!IsValidLivingTarget(TargetActor) || !IsThreatRecordRelevant(Record, CurrentTime))
		{
			continue;
		}

		if (const double* BlockedUntil = ReacquireBlockedUntil.Find(Pair.Key))
		{
			if (*BlockedUntil > CurrentTime)
			{
				continue;
			}
		}

		const float DistanceSq =
			FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation());

		const float DamageThreat = GetRecentDamageThreat(Record, CurrentTime);

		if (DamageThreat > BestDamageThreat ||
			(FMath::IsNearlyEqual(DamageThreat, BestDamageThreat) &&
				DistanceSq < BestDamageTargetDistanceSq))
		{
			BestDamageThreat = DamageThreat;
			BestDamageTarget = TargetActor;
			BestDamageTargetDistanceSq = DistanceSq;
		}

		if (DistanceSq < NearestDistanceSq)
		{
			NearestTarget = TargetActor;
			NearestDistanceSq = DistanceSq;
		}
	}

	return BestDamageThreat > 0.0f ? BestDamageTarget : NearestTarget;
}

float UNSEnemyThreatComponent::GetRecentDamageThreat(
	const FNSEnemyThreatRecord& Record,
	double CurrentTime
) const
{
	const double DamageCutoff = CurrentTime - DamageThreatWindow;

	float TotalDamage = 0.0f;

	for (const FNSEnemyThreatDamage& Sample : Record.DamageSamples)
	{
		if (Sample.Timestamp >= DamageCutoff)
		{
			TotalDamage += Sample.Damage;
		}
	}

	return TotalDamage;
}

bool UNSEnemyThreatComponent::IsThreatRecordRelevant(
	const FNSEnemyThreatRecord& Record,
	double CurrentTime
) const
{
	if (!Record.TargetActor.IsValid())
	{
		return false;
	}

	if (Record.bCurrentlyVisible)
	{
		return true;
	}

	if (Record.LastSeenTime >= 0.0 &&
		CurrentTime - Record.LastSeenTime <= SightMemoryDuration)
	{
		return true;
	}

	if (Record.LastStimulusTime >= 0.0 &&
		CurrentTime - Record.LastStimulusTime <= StimulusMemoryDuration)
	{
		return true;
	}

	return !Record.DamageSamples.IsEmpty();
}

bool UNSEnemyThreatComponent::ShouldSwitchTarget(
	AActor* CandidateTarget,
	double CurrentTime
) const
{
	AActor* CurrentTargetActor = CurrentTarget.Get();

	if (!CandidateTarget || CandidateTarget == CurrentTargetActor)
	{
		return false;
	}

	if (!IsValidLivingTarget(CurrentTargetActor))
	{
		return true;
	}

	if (CurrentTime - LastTargetSwitchTime < TargetSwitchCooldown)
	{
		return false;
	}

	const bool bInitialLockFinished =
		bAttackStartedOnCurrentTarget ||
		CurrentTime - CurrentTargetSelectedTime >= InitialTargetLockDuration;

	if (!bInitialLockFinished)
	{
		return false;
	}

	const FNSEnemyThreatRecord* CurrentRecord = ThreatRecords.Find(CurrentTargetActor);
	const FNSEnemyThreatRecord* CandidateRecord = ThreatRecords.Find(CandidateTarget);

	if (!CandidateRecord)
	{
		return false;
	}

	if (!CurrentRecord)
	{
		return true;
	}

	const float CurrentDamage = GetRecentDamageThreat(*CurrentRecord, CurrentTime);
	const float CandidateDamage = GetRecentDamageThreat(*CandidateRecord, CurrentTime);

	if (CandidateDamage > 0.0f)
	{
		if (CurrentDamage <= 0.0f)
		{
			return true;
		}

		return CandidateDamage >= CurrentDamage * DamageThreatSwitchRatio;
	}

	if (CurrentDamage > 0.0f)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}

	const float CurrentDistance =
		FVector::Dist(OwnerPawn->GetActorLocation(), CurrentTargetActor->GetActorLocation());

	const float CandidateDistance =
		FVector::Dist(OwnerPawn->GetActorLocation(), CandidateTarget->GetActorLocation());

	return CandidateDistance <= CurrentDistance * DistanceSwitchRatio;
}

bool UNSEnemyThreatComponent::IsValidLivingTarget(const AActor* Target) const
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

void UNSEnemyThreatComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (!IsValidLivingTarget(NewTarget) || CurrentTarget.Get() == NewTarget)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CurrentTarget = NewTarget;

	const double CurrentTime = World->GetTimeSeconds();

	CurrentTargetSelectedTime = CurrentTime;
	LastTargetSwitchTime = CurrentTime;
	LastCombatProgressTime = CurrentTime;

	bAttackStartedOnCurrentTarget = false;
}
