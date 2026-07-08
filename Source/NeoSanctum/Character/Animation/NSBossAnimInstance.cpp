// Copyright 2026 One Team. All rights reserved.

#include "NSBossAnimInstance.h"

#include "GameFramework/Pawn.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"

void UNSBossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwner();
	UpdateState();
}

void UNSBossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(OwnerPawn) || !IsValid(StateComponent) || !EnemyAgent.GetObject())
	{
		CacheOwner();
	}

	UpdateState();
}

void UNSBossAnimInstance::ResetCombatAimImmediate()
{
}

void UNSBossAnimInstance::CacheOwner()
{
	OwnerPawn = TryGetPawnOwner();

	EnemyAgent.SetObject(nullptr);
	EnemyAgent.SetInterface(nullptr);
	StateComponent = nullptr;

	if (!IsValid(OwnerPawn))
	{
		return;
	}

	if (INSEnemyAgent* NewEnemyAgent = Cast<INSEnemyAgent>(OwnerPawn))
	{
		EnemyAgent.SetObject(OwnerPawn);
		EnemyAgent.SetInterface(NewEnemyAgent);
	}

	StateComponent = OwnerPawn->FindComponentByClass<UNSEnemyStateComponent>();
}

void UNSBossAnimInstance::UpdateState()
{
	if (!IsValid(StateComponent))
	{
		bIsDead = false;
		bIsHitReacting = false;
		return;
	}

	bIsDead = StateComponent->IsDead();
	bIsHitReacting = StateComponent->IsHitReacting();
}
