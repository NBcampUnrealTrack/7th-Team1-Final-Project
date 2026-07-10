// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyCombatComponent.h"

#include "Net/UnrealNetwork.h"

UNSEnemyCombatComponent::UNSEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSEnemyCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSEnemyCombatComponent, ReplicatedAttackId);
	DOREPLIFETIME(UNSEnemyCombatComponent, bHasReplicatedAimTargetLocation);
	DOREPLIFETIME(UNSEnemyCombatComponent, ReplicatedAimTargetLocation);
}

void UNSEnemyCombatComponent::SetAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	CurrentAttackRow = InAttackRow;
	bHasCurrentAttackRow = true;
	ReplicatedAttackId = InAttackRow.AttackId;
}

const FNSEnemyAttackRow* UNSEnemyCombatComponent::GetAttackRow() const
{
	return bHasCurrentAttackRow ? &CurrentAttackRow : nullptr;
}

void UNSEnemyCombatComponent::ClearAttackRow()
{
	CurrentAttackRow = FNSEnemyAttackRow();
	bHasCurrentAttackRow = false;
	ReplicatedAttackId = NAME_None;

	ClearReplicatedAimTargetLocation();
}

FName UNSEnemyCombatComponent::GetCurrentAttackId() const
{
	if (bHasCurrentAttackRow)
	{
		return CurrentAttackRow.AttackId;
	}

	return ReplicatedAttackId;
}

void UNSEnemyCombatComponent::SetReplicatedAimTargetLocation(const FVector& InLocation)
{
	if (const AActor* OwnerActor = GetOwner(); !OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	bHasReplicatedAimTargetLocation = true;
	ReplicatedAimTargetLocation = InLocation;
}

void UNSEnemyCombatComponent::ClearReplicatedAimTargetLocation()
{
	if (const AActor* OwnerActor = GetOwner(); !OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	bHasReplicatedAimTargetLocation = false;
	ReplicatedAimTargetLocation = FVector::ZeroVector;
}

bool UNSEnemyCombatComponent::TryGetReplicatedAimTargetLocation(FVector& OutLocation) const
{
	if (!bHasReplicatedAimTargetLocation)
	{
		return false;
	}

	OutLocation = ReplicatedAimTargetLocation;
	return true;
}
