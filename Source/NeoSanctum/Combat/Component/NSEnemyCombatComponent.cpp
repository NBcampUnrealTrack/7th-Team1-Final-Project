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
}

FName UNSEnemyCombatComponent::GetCurrentAttackId() const
{
	if (bHasCurrentAttackRow)
	{
		return CurrentAttackRow.AttackId;
	}

	return ReplicatedAttackId;
}
