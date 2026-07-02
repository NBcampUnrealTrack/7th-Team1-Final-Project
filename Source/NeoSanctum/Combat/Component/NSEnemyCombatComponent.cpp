// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyCombatComponent.h"

UNSEnemyCombatComponent::UNSEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSEnemyCombatComponent::SetAttackRow(const FNSEnemyAttackRow& InAttackRow)
{
	CurrentAttackRow = InAttackRow;
	bHasCurrentAttackRow = true;
}

const FNSEnemyAttackRow* UNSEnemyCombatComponent::GetAttackRow() const
{
	return bHasCurrentAttackRow ? &CurrentAttackRow : nullptr;
}

void UNSEnemyCombatComponent::ClearAttackRow()
{
	CurrentAttackRow = FNSEnemyAttackRow();
	bHasCurrentAttackRow = false;
}

FName UNSEnemyCombatComponent::GetCurrentAttackId() const
{
	return bHasCurrentAttackRow ? CurrentAttackRow.AttackId : NAME_None;
}
