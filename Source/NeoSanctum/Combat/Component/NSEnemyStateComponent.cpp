// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyStateComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

UNSEnemyStateComponent::UNSEnemyStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSEnemyStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSEnemyStateComponent, bDead);
	DOREPLIFETIME(UNSEnemyStateComponent, bInactive);
	DOREPLIFETIME(UNSEnemyStateComponent, bHitReacting);
}

void UNSEnemyStateComponent::InitState(UNSMonsterAttributeSet* InMonsterAttributes)
{
	MonsterAttributes = InMonsterAttributes;
}

void UNSEnemyStateComponent::Die(AController* Killer)
{
	if (!IsOwnerAuthority() || bDead)
	{
		return;
	}

	bDead = true;
	LastKiller = Killer;

	FinishHitReaction();
	ResetHitGauge();
	ClearOwnerAttackRow();

	OnDeadStateChanged.Broadcast(bDead);
	OnDeathStarted.Broadcast();

	UNSEnemyData* EnemyData = GetOwnerEnemyData();
	UAbilitySystemComponent* ASC = GetOwnerASC();

	if (ASC && EnemyData && EnemyData->DeathAbilityClass)
	{
		ASC->TryActivateAbilityByClass(EnemyData->DeathAbilityClass);
	}
}

void UNSEnemyStateComponent::SetInactive(bool bNewInactive)
{
	if (!IsOwnerAuthority() || bInactive == bNewInactive)
	{
		return;
	}

	bInactive = bNewInactive;

	if (bInactive)
	{
		FinishHitReaction();
		ResetHitGauge();
		ClearOwnerAttackRow();
	}

	OnInactiveStateChanged.Broadcast(bInactive);
}

void UNSEnemyStateComponent::ResetForReuse()
{
	if (!IsOwnerAuthority())
	{
		return;
	}

	const bool bWasDead = bDead;
	const bool bWasInactive = bInactive;
	const bool bWasHitReacting = bHitReacting;

	bDead = false;
	bInactive = false;
	bHitReacting = false;

	ResetHitGauge();

	if (bWasDead)
	{
		OnDeadStateChanged.Broadcast(false);
	}

	if (bWasInactive)
	{
		OnInactiveStateChanged.Broadcast(false);
	}

	if (bWasHitReacting)
	{
		OnHitReactionStateChanged.Broadcast(false);
	}
}

void UNSEnemyStateComponent::StartHitReaction()
{
	if (!CanReceiveHitGauge())
	{
		return;
	}

	UNSEnemyData* EnemyData = GetOwnerEnemyData();
	UAbilitySystemComponent* ASC = GetOwnerASC();

	if (!ASC || !EnemyData || !EnemyData->HitReactionAbilityClass)
	{
		return;
	}

	SetHitReactionState(true);

	const bool bActivated = ASC->TryActivateAbilityByClass(EnemyData->HitReactionAbilityClass);

	if (!bActivated)
	{
		FinishHitReaction();
	}
}

void UNSEnemyStateComponent::FinishHitReaction()
{
	if (!IsOwnerAuthority() || !bHitReacting)
	{
		return;
	}

	SetHitReactionState(false);
}

void UNSEnemyStateComponent::ResetHitGauge()
{
	if (!IsOwnerAuthority() || !MonsterAttributes)
	{
		return;
	}

	MonsterAttributes->ResetHitGauge();
}

float UNSEnemyStateComponent::GetHitGauge() const
{
	return MonsterAttributes ? MonsterAttributes->GetHitGauge() : 0.0f;
}

float UNSEnemyStateComponent::GetMaxHitGauge() const
{
	return MonsterAttributes ? MonsterAttributes->GetMaxHitGauge() : 0.0f;
}

bool UNSEnemyStateComponent::CanReceiveHitGauge() const
{
	return IsOwnerAuthority() && !bDead && !bInactive && !bHitReacting;
}

void UNSEnemyStateComponent::OnRep_bDead()
{
	OnDeadStateChanged.Broadcast(bDead);
}

void UNSEnemyStateComponent::OnRep_bInactive()
{
	OnInactiveStateChanged.Broadcast(bInactive);
}

void UNSEnemyStateComponent::OnRep_bHitReacting()
{
	OnHitReactionStateChanged.Broadcast(bHitReacting);
}

void UNSEnemyStateComponent::SetHitReactionState(bool bNewHitReacting)
{
	if (!IsOwnerAuthority() || bHitReacting == bNewHitReacting)
	{
		return;
	}

	bHitReacting = bNewHitReacting;
	OnHitReactionStateChanged.Broadcast(bHitReacting);
}

bool UNSEnemyStateComponent::IsOwnerAuthority() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor && OwnerActor->HasAuthority();
}

UAbilitySystemComponent* UNSEnemyStateComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

UNSEnemyData* UNSEnemyStateComponent::GetOwnerEnemyData() const
{
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());
	return EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;
}

void UNSEnemyStateComponent::ClearOwnerAttackRow() const
{
	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetOwner());

	if (EnemyAgent)
	{
		EnemyAgent->ClearCurrentAttackRow();
	}
}
