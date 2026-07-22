// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyMeleeComponent.h"

#include "Abilities/GameplayAbility.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Component/NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSEnemyMeleeComponent::UNSEnemyMeleeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSEnemyMeleeComponent::ResetMeleeState()
{
	ReleaseReservation(false);
	ReservationTarget.Reset();
}

FNSMeleeState UNSEnemyMeleeComponent::RequestReservation(
	AActor* TargetActor,
	double LastDamagedTime
)
{
	FNSMeleeState State;

	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	if (!Enemy || !TargetActor || !UsesReservation())
	{
		return State;
	}

	UNSMeleeAttackReservationComponent* Component = GetReservationComponent(TargetActor);

	if (!Component)
	{
		ReleaseReservation(false);

		State.bAccepted = true;
		State.bHasReservation = false;
		State.bCanApproach = true;
		return State;
	}

	if (ReservationTarget.IsValid() && ReservationTarget.Get() != TargetActor)
	{
		ReleaseReservation(false);
	}

	ReservationTarget = TargetActor;

	const ENSMeleeReservationRequestResult Result =
		Component->RequestReservation(Enemy, LastDamagedTime);

	const bool bReserved = Result == ENSMeleeReservationRequestResult::Reserved;

	State.bAccepted = Result != ENSMeleeReservationRequestResult::Rejected;
	State.bHasReservation = bReserved;
	State.bCanApproach = bReserved;

	return State;
}

bool UNSEnemyMeleeComponent::HasReservation() const
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	AActor* TargetActor = ReservationTarget.Get();

	if (!Enemy || !TargetActor)
	{
		return false;
	}

	UNSMeleeAttackReservationComponent* Component = GetReservationComponent(TargetActor);

	return Component && Component->HasReservation(Enemy);
}

bool UNSEnemyMeleeComponent::CanApproachTarget(AActor* TargetActor) const
{
	if (!UsesReservation())
	{
		return true;
	}

	if (!TargetActor)
	{
		return false;
	}

	const bool bReservationRequired = GetReservationComponent(TargetActor) != nullptr;

	return !bReservationRequired || HasReservation();
}

bool UNSEnemyMeleeComponent::TargetRequiresReservation(AActor* TargetActor) const
{
	return UsesReservation() && TargetActor && GetReservationComponent(TargetActor) != nullptr;
}

void UNSEnemyMeleeComponent::NotifyAttackStarted()
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	AActor* TargetActor = ReservationTarget.Get();

	if (!Enemy || !TargetActor)
	{
		return;
	}

	if (UNSMeleeAttackReservationComponent* Component = GetReservationComponent(TargetActor))
	{
		Component->MarkAttackStarted(Enemy);
	}
}

void UNSEnemyMeleeComponent::MarkAttackInterrupted()
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	AActor* TargetActor = ReservationTarget.Get();

	if (!Enemy || !TargetActor)
	{
		return;
	}

	if (UNSMeleeAttackReservationComponent* Component = GetReservationComponent(TargetActor))
	{
		Component->MarkAttackInterrupted(Enemy);
	}
}

void UNSEnemyMeleeComponent::ReleaseReservation(bool bStartReacquireCooldown)
{
	ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	AActor* TargetActor = ReservationTarget.Get();

	if (Enemy && TargetActor)
	{
		if (UNSMeleeAttackReservationComponent* Component = GetReservationComponent(TargetActor))
		{
			Component->ReleaseReservation(Enemy, bStartReacquireCooldown);
		}
	}

	ReservationTarget.Reset();
}

FNSMeleeState UNSEnemyMeleeComponent::UpdateState(AActor* CurrentTarget)
{
	FNSMeleeState State;

	if (!UsesReservation())
	{
		State.bAccepted = true;
		State.bHasReservation = false;
		State.bCanApproach = true;
		return State;
	}

	if (!CurrentTarget)
	{
		ReleaseReservation(false);
		return State;
	}

	UNSMeleeAttackReservationComponent* Component = GetReservationComponent(CurrentTarget);

	if (!Component)
	{
		if (ReservationTarget.IsValid())
		{
			ReleaseReservation(false);
		}

		State.bAccepted = true;
		State.bHasReservation = false;
		State.bCanApproach = true;
		return State;
	}

	if (ReservationTarget.IsValid() && ReservationTarget.Get() != CurrentTarget)
	{
		ReleaseReservation(false);
		return State;
	}

	const bool bReserved = HasReservation();

	State.bAccepted = true;
	State.bHasReservation = bReserved;
	State.bCanApproach = bReserved;

	return State;
}

bool UNSEnemyMeleeComponent::UsesReservation() const
{
	const UNSEnemyData* EnemyData = GetEnemyData();

	if (!EnemyData)
	{
		return false;
	}

	for (const FNSEnemyAttackRow* AttackRow : EnemyData->GetAttackRows())
	{
		if (AttackRow &&
			AttackRow->AbilityClass &&
			AttackRow->AttackType == ENSEnemyAttackType::MeleeSweep)
		{
			return true;
		}
	}

	return false;
}

ANSEnemyCharacterBase* UNSEnemyMeleeComponent::GetOwnerEnemy() const
{
	return Cast<ANSEnemyCharacterBase>(GetOwner());
}

const UNSEnemyData* UNSEnemyMeleeComponent::GetEnemyData() const
{
	const ANSEnemyCharacterBase* Enemy = GetOwnerEnemy();
	return Enemy ? Enemy->GetEnemyData() : nullptr;
}

UNSMeleeAttackReservationComponent* UNSEnemyMeleeComponent::GetReservationComponent(
	AActor* TargetActor
) const
{
	return TargetActor
		       ? TargetActor->FindComponentByClass<UNSMeleeAttackReservationComponent>()
		       : nullptr;
}
