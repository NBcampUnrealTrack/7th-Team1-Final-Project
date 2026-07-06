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
#include "NeoSanctum/Type/NSBBTypes.h"

ANSEnemyControllerBase::ANSEnemyControllerBase()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ANSEnemyControllerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PrimaryActorTick.TickInterval = 0.1f;

	SyncCommonStateBlackboard();
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

	InitCommonBlackboardState();
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
		NSBB::Phase::PhasePatternLocked,
		PhaseComponent && PhaseComponent->IsPatternLocked());

	CachedBBComp->SetValueAsName(
		NSBB::Phase::CurrentPhaseId,
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


void ANSEnemyControllerBase::InitCommonBlackboardState()
{
	if (!CachedBBComp)
	{
		return;
	}

	ClearCommonTargetBB(true);

	SetIsAttackingBB(false);
	SetHitReactingBB(IsControlledEnemyHitReacting());

	CachedBBComp->SetValueAsBool(NSBB::Phase::PhasePatternLocked, false);
	CachedBBComp->SetValueAsName(NSBB::Phase::CurrentPhaseId, NAME_None);
}

void ANSEnemyControllerBase::SyncCommonStateBlackboard()
{
	SetHitReactingBB(IsControlledEnemyHitReacting());
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

void ANSEnemyControllerBase::SetCanAttackBB(bool bCanAttack)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(NSBB::Combat::CanAttack, bCanAttack);
	}
}

void ANSEnemyControllerBase::SetIsAttackingBB(bool bIsAttacking)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(NSBB::Combat::IsAttacking, bIsAttacking);
	}
}

void ANSEnemyControllerBase::SetHitReactingBB(bool bHitReacting)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(NSBB::Combat::IsHitReacting, bHitReacting);
	}
}

bool ANSEnemyControllerBase::IsAttackingBB() const
{
	return CachedBBComp && CachedBBComp->GetValueAsBool(NSBB::Combat::IsAttacking);
}

bool ANSEnemyControllerBase::IsHitReactingBB() const
{
	return CachedBBComp && CachedBBComp->GetValueAsBool(NSBB::Combat::IsHitReacting);
}

void ANSEnemyControllerBase::SetTargetActorBB(AActor* TargetActor)
{
	if (!CachedBBComp)
	{
		return;
	}

	if (IsValid(TargetActor))
	{
		CachedBBComp->SetValueAsObject(NSBB::Target::TargetActor, TargetActor);
	}
	else
	{
		CachedBBComp->ClearValue(NSBB::Target::TargetActor);
	}
}

void ANSEnemyControllerBase::SetAttackActorBB(AActor* AttackActor)
{
	if (!CachedBBComp)
	{
		return;
	}

	if (IsValid(AttackActor))
	{
		CachedBBComp->SetValueAsObject(NSBB::Target::AttackActor, AttackActor);
	}
	else
	{
		CachedBBComp->ClearValue(NSBB::Target::AttackActor);
	}
}

AActor* ANSEnemyControllerBase::GetTargetActorBB() const
{
	return CachedBBComp
		       ? Cast<AActor>(CachedBBComp->GetValueAsObject(NSBB::Target::TargetActor))
		       : nullptr;
}

AActor* ANSEnemyControllerBase::GetAttackActorBB() const
{
	return CachedBBComp
		       ? Cast<AActor>(CachedBBComp->GetValueAsObject(NSBB::Target::AttackActor))
		       : nullptr;
}

void ANSEnemyControllerBase::SetTargetLastKnownLocationBB(const FVector& LastKnownLocation)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsVector(
			NSBB::Target::TargetLastKnownLocation,
			LastKnownLocation);
	}
}

void ANSEnemyControllerBase::SetHasTargetLineOfSightBB(bool bHasLineOfSight)
{
	if (CachedBBComp)
	{
		CachedBBComp->SetValueAsBool(
			NSBB::Target::HasTargetLineOfSight,
			bHasLineOfSight);
	}
}

void ANSEnemyControllerBase::ClearCommonTargetBB(bool bClearCanAttack)
{
	if (!CachedBBComp)
	{
		return;
	}

	CachedBBComp->ClearValue(NSBB::Target::TargetActor);
	CachedBBComp->ClearValue(NSBB::Target::AttackActor);
	CachedBBComp->ClearValue(NSBB::Target::TargetLastKnownLocation);
	CachedBBComp->SetValueAsBool(NSBB::Target::HasTargetLineOfSight, false);

	if (bClearCanAttack)
	{
		SetCanAttackBB(false);
	}
}
