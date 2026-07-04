// Copyright 2026 One Team. All rights reserved.

#include "NSEnemyControllerBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "NeoSanctum/AI/Components/NSFlyingLocomotionComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyPhaseComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"

ANSEnemyControllerBase::ANSEnemyControllerBase()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ANSEnemyControllerBase::NotifyAttackStarted()
{
	SetIsAttackingBB(true);
}

void ANSEnemyControllerBase::NotifyAttackFinished()
{
	ClearControlledAttackRow();
	SetIsAttackingBB(false);
}

void ANSEnemyControllerBase::StartEnemyBrain(const UNSEnemyData* EnemyData)
{
	if (!EnemyData)
	{
		return;
	}

	StopEnemyBrain(TEXT("Restart Enemy Brain"));

	switch (EnemyData->BrainType)
	{
	case ENSEnemyBrainType::BehaviorTree:
	{
		if (!EnemyData->BehaviorTree)
		{
			UE_LOG(LogTemp, Warning, TEXT("BehaviorTree 타입인데, 비어 있음."));
			return;
		}

		RunBehaviorTree(EnemyData->BehaviorTree);
		CachedBBComp = GetBlackboardComponent();
		break;
	}

	case ENSEnemyBrainType::StateTree:
	{
		if (!StateTreeComponent || !EnemyData->StateTree)
		{
			UE_LOG(LogTemp, Warning, TEXT("StateTree 타입인데, 비어 있음."));
			return;
		}

		BrainComponent = StateTreeComponent;
		StateTreeComponent->SetStateTree(EnemyData->StateTree);
		StateTreeComponent->StartLogic();

		CachedBBComp = GetBlackboardComponent();
		break;
	}
	}
}

void ANSEnemyControllerBase::StopEnemyBrain(const FString& Reason)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(Reason);
	}
}

const INSEnemyAgent* ANSEnemyControllerBase::GetControlledEnemyAgent() const
{
	return Cast<INSEnemyAgent>(GetPawn());
}

const UNSEnemyData* ANSEnemyControllerBase::GetControlledEnemyData() const
{
	const INSEnemyAgent* EnemyAgent = GetControlledEnemyAgent();
	return EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;
}

UAbilitySystemComponent* ANSEnemyControllerBase::GetControlledEnemyASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetPawn());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

bool ANSEnemyControllerBase::IsControlledEnemyHitReacting() const
{
	const INSEnemyAgent* EnemyAgent = GetControlledEnemyAgent();
	return EnemyAgent && EnemyAgent->IsHitReacting();
}

float ANSEnemyControllerBase::GetControlledEnemyHealthRatio() const
{
	const UAbilitySystemComponent* ASC = GetControlledEnemyASC();
	if (!ASC)
	{
		return 1.0f;
	}

	const UNSMonsterAttributeSet* MonsterAttributes =
		ASC->GetSet<UNSMonsterAttributeSet>();

	if (!MonsterAttributes)
	{
		return 1.0f;
	}

	const float MaxHealth = FMath::Max(MonsterAttributes->GetMaxHealth(), 1.0f);
	const float Health = FMath::Clamp(MonsterAttributes->GetHealth(), 0.0f, MaxHealth);

	return Health / MaxHealth;
}

void ANSEnemyControllerBase::UpdateEnemyPhase()
{
	UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent();
	if (!PhaseComponent)
	{
		SyncPhaseBlackboard(nullptr);
		return;
	}

	PhaseComponent->UpdatePhase(GetControlledEnemyHealthRatio());
	SyncPhaseBlackboard(PhaseComponent);
}

bool ANSEnemyControllerBase::IsPhasePatternLocked() const
{
	const UNSEnemyPhaseComponent* PhaseComponent = GetEnemyPhaseComponent();
	return PhaseComponent && PhaseComponent->IsPatternLocked();
}

UNSEnemyPhaseComponent* ANSEnemyControllerBase::GetEnemyPhaseComponent() const
{
	const APawn* ControlledPawn = GetPawn();
	return ControlledPawn
		? ControlledPawn->FindComponentByClass<UNSEnemyPhaseComponent>()
		: nullptr;
}

void ANSEnemyControllerBase::SyncPhaseBlackboard(
	const UNSEnemyPhaseComponent* PhaseComponent)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->SetValueAsBool(
		PhasePatternLockedKey,
		PhaseComponent && PhaseComponent->IsPatternLocked());

	CachedBBComp->SetValueAsName(
		CurrentPhaseIdKey,
		PhaseComponent ? PhaseComponent->GetCurrentPhaseId() : NAME_None);
}

UNSFlyingLocomotionComponent* ANSEnemyControllerBase::GetControlledFlyingLocomotion() const
{
	ANSEnemyPawnBase* EnemyPawn = Cast<ANSEnemyPawnBase>(GetPawn());
	if (!EnemyPawn) return nullptr;
	
	return EnemyPawn->GetFlyingLocomotion();
}

void ANSEnemyControllerBase::SyncFlyingRotationTarget(AActor* Target)
{
	UNSFlyingLocomotionComponent* FlyMovementComponent = GetControlledFlyingLocomotion();
	if (!FlyMovementComponent) return;
	
	FlyMovementComponent->SetRotationTarget(Target);
}


void ANSEnemyControllerBase::ClearControlledAttackRow()
{
	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(GetPawn());
	if (!EnemyAgent)
	{
		return;
	}

	EnemyAgent->ClearCurrentAttackRow();
}

void ANSEnemyControllerBase::SetIsAttackingBB(bool bIsAttacking)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(IsAttackingKey, bIsAttacking);
	}
}

bool ANSEnemyControllerBase::IsAttackingBB() const
{
	return CachedBBComp && CachedBBComp->GetValueAsBool(IsAttackingKey);
}
