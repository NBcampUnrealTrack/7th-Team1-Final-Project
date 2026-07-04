// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_ExecuteEnemyAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSBTTask_ExecuteEnemyAbility::UNSBTTask_ExecuteEnemyAbility()
{
	NodeName = TEXT("Execute Enemy Ability");

	// Behavior Tree 노드를 AI마다 개별 인스턴스로 생성해 상태값을 안전하게 저장 (덮어쓰기 방지)
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UNSBTTask_ExecuteEnemyAbility::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	ANSEnemyAIController* AIController = Cast<ANSEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* TargetPawn = AIController->GetPawn();
	if (!TargetPawn)
	{
		return EBTNodeResult::Failed;
	}

	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(TargetPawn);
	if (!EnemyAgent)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn);

	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	const FNSEnemyAttackRow* SelectedAttack = AIController->GetAttackRowByDistance();
	if (!SelectedAttack)
	{
		return EBTNodeResult::Failed;
	}

	TSubclassOf<UGameplayAbility> AttackAbilityClass = SelectedAttack->AbilityClass;
	if (!AttackAbilityClass)
	{
		return EBTNodeResult::Failed;
	}

	bUsesMeleeReservation =
		SelectedAttack->AttackType == ENSEnemyAttackType::MeleeSweep &&
		AIController->CurrentTargetRequiresMeleeReservation();

	if (bUsesMeleeReservation && !AIController->HasMeleeAttackReservation())
	{
		return EBTNodeResult::Failed;
	}

	EnemyAgent->SetCurrentAttackRow(*SelectedAttack);

	CachedOwnerComp = &OwnerComp;
	CachedAttackAbilityClass = AttackAbilityClass;

	ASC->OnAbilityEnded.AddUObject(
		this,
		&UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded);

	const bool bActivated = ASC->TryActivateAbilityByClass(AttackAbilityClass);
	if (!bActivated)
	{
		if (bUsesMeleeReservation)
		{
			AIController->ReleaseMeleeAttackReservation(true);
			bUsesMeleeReservation = false;
		}

		AIController->NotifyAttackFinished();

		ASC->OnAbilityEnded.RemoveAll(this);
		CachedAttackAbilityClass = nullptr;
		CachedOwnerComp = nullptr;

		return EBTNodeResult::Failed;
	}

	if (bUsesMeleeReservation)
	{
		AIController->NotifyMeleeReservationAttackStarted();
	}

	AIController->RecordAttackUsed(*SelectedAttack);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UNSBTTask_ExecuteEnemyAbility::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	ANSEnemyAIController* Controller = Cast<ANSEnemyAIController>(OwnerComp.GetAIOwner());

	APawn* EnemyPawn = Controller ? Controller->GetPawn() : nullptr;

	UAbilitySystemComponent* ASC = EnemyPawn
		                               ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EnemyPawn)
		                               : nullptr;

	if (ASC)
	{
		ASC->OnAbilityEnded.RemoveAll(this);

		if (CachedAttackAbilityClass)
		{
			if (const FGameplayAbilitySpec* Spec =
				ASC->FindAbilitySpecFromClass(CachedAttackAbilityClass))
			{
				ASC->CancelAbilityHandle(Spec->Handle);
			}
		}
	}

	const bool bPreserveMeleeReservation =
		ShouldPreserveMeleeReservation(Controller);

	if (Controller && bUsesMeleeReservation && !bPreserveMeleeReservation)
	{
		Controller->ReleaseMeleeAttackReservation(true);
	}

	if (Controller)
	{
		Controller->NotifyAttackFinished();
	}

	bUsesMeleeReservation = false;
	CachedAttackAbilityClass = nullptr;
	CachedOwnerComp = nullptr;

	return EBTNodeResult::Aborted;
}

void UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded(
	const FAbilityEndedData& AbilityEndedData)
{
	if (!CachedOwnerComp)
	{
		return;
	}

	if (CachedAttackAbilityClass &&
		AbilityEndedData.AbilityThatEnded &&
		!AbilityEndedData.AbilityThatEnded->IsA(CachedAttackAbilityClass))
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp;

	ANSEnemyAIController* AIController =
		Cast<ANSEnemyAIController>(OwnerComp->GetAIOwner());

	if (AIController)
	{
		if (APawn* TargetPawn = AIController->GetPawn())
		{
			if (UAbilitySystemComponent* ASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn))
			{
				ASC->OnAbilityEnded.RemoveAll(this);
			}
		}

		const bool bPreserveMeleeReservation =
			ShouldPreserveMeleeReservation(AIController);

		if (bUsesMeleeReservation && !bPreserveMeleeReservation)
		{
			AIController->ReleaseMeleeAttackReservation(true);
		}

		AIController->NotifyAttackFinished();
	}

	bUsesMeleeReservation = false;
	CachedAttackAbilityClass = nullptr;
	CachedOwnerComp = nullptr;

	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

bool UNSBTTask_ExecuteEnemyAbility::ShouldPreserveMeleeReservation(const ANSEnemyAIController* AIController) const
{
	if (!bUsesMeleeReservation || !AIController)
	{
		return false;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(AIController->GetPawn());

	return EnemyAgent &&
		EnemyAgent->IsHitReacting() &&
		AIController->HasMeleeAttackReservation();
}
