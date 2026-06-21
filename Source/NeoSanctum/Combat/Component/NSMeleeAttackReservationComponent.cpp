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
