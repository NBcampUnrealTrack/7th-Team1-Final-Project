// Copyright 2026 One Team. All rights reserved.

#include "NSBossAbilityExecutorComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSBossAbilityExecutorComponent::UNSBossAbilityExecutorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNSBossAbilityExecutorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelCurrentAttack();
	UnbindAbilityEndedDelegate();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoverTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool UNSBossAbilityExecutorComponent::RequestAttack(FName FixedAttackId)
{
	if (IsExecuting())
	{
		return false;
	}

	ResetRuntimeForNewAttack();

	ANSBossAIController* BossController = GetBossController();
	if (!BossController)
	{
		ExecutionState = ENSBossAbilityExecutionState::Failed;
		return false;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		ExecutionState = ENSBossAbilityExecutionState::Failed;
		return false;
	}

	const FNSEnemyAttackRow* SelectedAttack =
		SelectAttackRow(BossController, FixedAttackId);

	if (!SelectedAttack || !SelectedAttack->AbilityClass)
	{
		ExecutionState = ENSBossAbilityExecutionState::Failed;
		return false;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerPawn);

	if (!EnemyAgent)
	{
		ExecutionState = ENSBossAbilityExecutionState::Failed;
		return false;
	}

	EnemyAgent->SetCurrentAttackRow(*SelectedAttack);

	RequestedFixedAttackId = FixedAttackId;
	CurrentAttackId = SelectedAttack->AttackId;
	CurrentAbilityClass = SelectedAttack->AbilityClass;
	RecoverTime = FMath::Max(SelectedAttack->RecoverTime, 0.0f);
	CachedBossController = BossController;
	CachedASC = ASC;

	return ActivateAttackAbility(BossController, ASC, *SelectedAttack);
}

void UNSBossAbilityExecutorComponent::CancelCurrentAttack()
{
	if (!IsExecuting())
	{
		return;
	}

	if (ExecutionState == ENSBossAbilityExecutionState::Recovering)
	{
		FinishAttack(ENSBossAbilityExecutionState::Cancelled);
		return;
	}

	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CurrentAbilityClass)
	{
		FinishAttack(ENSBossAbilityExecutionState::Cancelled);
		return;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(CurrentAbilityClass);
	if (Spec && Spec->IsActive())
	{
		ASC->CancelAbilityHandle(Spec->Handle);
		return;
	}

	FinishAttack(ENSBossAbilityExecutionState::Cancelled);
}

void UNSBossAbilityExecutorComponent::ResetExecutor()
{
	if (IsExecuting())
	{
		CancelCurrentAttack();
	}

	UnbindAbilityEndedDelegate();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoverTimerHandle);
	}

	ExecutionState = ENSBossAbilityExecutionState::Idle;
	CurrentAttackId = NAME_None;
	RequestedFixedAttackId = NAME_None;
	RecoverTime = 0.0f;
	bAttackFinishNotified = false;
	ClearRuntimeCache();
}

bool UNSBossAbilityExecutorComponent::IsExecuting() const
{
	if (ExecutionState == ENSBossAbilityExecutionState::Running ||
		ExecutionState == ENSBossAbilityExecutionState::Recovering)
	{
		return true;
	}

	return IsCurrentAbilityActive();
}

bool UNSBossAbilityExecutorComponent::WasLastAttackFailed() const
{
	return ExecutionState == ENSBossAbilityExecutionState::Failed ||
		ExecutionState == ENSBossAbilityExecutionState::Cancelled;
}

ANSBossAIController* UNSBossAbilityExecutorComponent::GetBossController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	return Cast<ANSBossAIController>(OwnerPawn->GetController());
}

UAbilitySystemComponent* UNSBossAbilityExecutorComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}

const FNSEnemyAttackRow* UNSBossAbilityExecutorComponent::SelectAttackRow(
	ANSBossAIController* BossController,
	FName FixedAttackId) const
{
	if (!BossController)
	{
		return nullptr;
	}

	return FixedAttackId.IsNone()
		       ? BossController->GetAttackRowByDistance()
		       : BossController->GetAttackRowById(FixedAttackId);
}

bool UNSBossAbilityExecutorComponent::ActivateAttackAbility(
	ANSBossAIController* BossController,
	UAbilitySystemComponent* ASC,
	const FNSEnemyAttackRow& AttackRow)
{
	if (!BossController || !ASC || !AttackRow.AbilityClass)
	{
		FinishAttack(ENSBossAbilityExecutionState::Failed);
		return false;
	}

	UnbindAbilityEndedDelegate();

	ASC->OnAbilityEnded.AddUObject(
		this,
		&UNSBossAbilityExecutorComponent::OnAttackAbilityEnded);

	ExecutionState = ENSBossAbilityExecutionState::Running;

	BossController->NotifyAttackStarted();

	const bool bActivated = ASC->TryActivateAbilityByClass(AttackRow.AbilityClass);
	if (!bActivated)
	{
		FinishAttack(ENSBossAbilityExecutionState::Failed);
		return false;
	}

	BossController->RecordAttackCommitted(AttackRow);

	return true;
}

void UNSBossAbilityExecutorComponent::OnAttackAbilityEnded(
	const FAbilityEndedData& AbilityEndedData)
{
	if (!CurrentAbilityClass ||
		!AbilityEndedData.AbilityThatEnded ||
		!AbilityEndedData.AbilityThatEnded->IsA(CurrentAbilityClass))
	{
		return;
	}

	if (AbilityEndedData.bWasCancelled)
	{
		FinishAttack(ENSBossAbilityExecutionState::Cancelled);
		return;
	}

	if (RecoverTime > 0.0f)
	{
		StartRecover(RecoverTime);
		return;
	}

	FinishAttack(ENSBossAbilityExecutionState::Succeeded);
}

void UNSBossAbilityExecutorComponent::StartRecover(float InRecoverTime)
{
	if (InRecoverTime <= 0.0f)
	{
		FinishAttack(ENSBossAbilityExecutionState::Succeeded);
		return;
	}

	UnbindAbilityEndedDelegate();

	ExecutionState = ENSBossAbilityExecutionState::Recovering;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RecoverTimerHandle,
			this,
			&UNSBossAbilityExecutorComponent::CompleteRecover,
			InRecoverTime,
			false);

		return;
	}

	FinishAttack(ENSBossAbilityExecutionState::Succeeded);
}

void UNSBossAbilityExecutorComponent::CompleteRecover()
{
	FinishAttack(ENSBossAbilityExecutionState::Succeeded);
}

void UNSBossAbilityExecutorComponent::FinishAttack(
	ENSBossAbilityExecutionState FinishState)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoverTimerHandle);
	}

	UnbindAbilityEndedDelegate();

	if (!bAttackFinishNotified)
	{
		if (ANSBossAIController* BossController = CachedBossController.Get())
		{
			BossController->NotifyAttackFinished();
		}

		bAttackFinishNotified = true;
	}

	ExecutionState = FinishState;
	ClearRuntimeCache();
}

bool UNSBossAbilityExecutorComponent::IsCurrentAbilityActive() const
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CurrentAbilityClass)
	{
		return false;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(CurrentAbilityClass);

	return Spec && Spec->IsActive();
}

void UNSBossAbilityExecutorComponent::UnbindAbilityEndedDelegate()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.RemoveAll(this);
	}
}

void UNSBossAbilityExecutorComponent::ResetRuntimeForNewAttack()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoverTimerHandle);
	}

	UnbindAbilityEndedDelegate();

	ExecutionState = ENSBossAbilityExecutionState::Idle;
	CurrentAttackId = NAME_None;
	RequestedFixedAttackId = NAME_None;
	RecoverTime = 0.0f;
	bAttackFinishNotified = false;
	CurrentAbilityClass = nullptr;
	CachedBossController.Reset();
	CachedASC.Reset();
}

void UNSBossAbilityExecutorComponent::ClearRuntimeCache()
{
	RequestedFixedAttackId = NAME_None;
	RecoverTime = 0.0f;
	CurrentAbilityClass = nullptr;
	CachedBossController.Reset();
	CachedASC.Reset();
}
