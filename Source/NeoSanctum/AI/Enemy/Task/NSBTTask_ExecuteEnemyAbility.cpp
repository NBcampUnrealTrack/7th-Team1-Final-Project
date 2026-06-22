// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_ExecuteEnemyAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"

UNSBTTask_ExecuteEnemyAbility::UNSBTTask_ExecuteEnemyAbility()
{
	NodeName = TEXT("Execute Enemy Ability");

	// Behavior Tree 노드를 AI마다 개별 인스턴스로 생성해 상태값을 안전하게 저장 (덮어쓰기 방지)
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UNSBTTask_ExecuteEnemyAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANSEnemyAIController* AIController = Cast<ANSEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController) return EBTNodeResult::Failed;

	APawn* TargetPawn = AIController->GetPawn();
	if (!TargetPawn) return EBTNodeResult::Failed;
	
	ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(TargetPawn);
	if (!Enemy) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn);
	if (!ASC) return EBTNodeResult::Failed;
	
	const FNSEnemyAttackDefinition* SelectedAttack = AIController->GetAttackDefinitionByDistance();
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
	
	Enemy->SetCurrentAttackDefinition(*SelectedAttack);
	
	CachedOwnerComp = &OwnerComp;
	CachedAttackAbilityClass = AttackAbilityClass;
	
	ASC->OnAbilityEnded.AddUObject(this, &UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded);

	bool bActivated = ASC->TryActivateAbilityByClass(AttackAbilityClass);
	if (!bActivated)
	{
		if (bUsesMeleeReservation)
		{
			AIController->ReleaseMeleeAttackReservation(true);
			bUsesMeleeReservation = false;
		}
		
		Enemy->ClearCurrentAttackDefinition();
		ASC->OnAbilityEnded.RemoveAll(this);
		CachedAttackAbilityClass = nullptr;
		return EBTNodeResult::Failed;
	}
	
	if (bUsesMeleeReservation)
	{
		AIController->NotifyMeleeReservationAttackStarted();
	}
	
	AIController->RecordAttackUsed(*SelectedAttack);
	
	if (UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent())
	{
		BBComp->SetValueAsBool(TEXT("bIsAttacking"), true);
	}
	
	// 애니메이션이 끝날 때까지 대기
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UNSBTTask_ExecuteEnemyAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
			if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(CachedAttackAbilityClass))
			{
				ASC->CancelAbilityHandle(Spec->Handle);
			}
		}
	}

	if (ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(EnemyPawn))
	{
		Enemy->ClearCurrentAttackDefinition();
	}
	
	const bool bPreserveMeleeReservation = ShouldPreserveMeleeReservation(Controller);

	if (Controller && bUsesMeleeReservation && !bPreserveMeleeReservation)
	{
		Controller->ReleaseMeleeAttackReservation(true);
	}

	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		Blackboard->SetValueAsBool(TEXT("bIsAttacking"), false);
	}

	bUsesMeleeReservation = false;
	CachedAttackAbilityClass = nullptr;
	CachedOwnerComp = nullptr;

	return EBTNodeResult::Aborted;
}

void UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!CachedOwnerComp) return;
	
	if (CachedAttackAbilityClass &&
		AbilityEndedData.AbilityThatEnded &&
		!AbilityEndedData.AbilityThatEnded->IsA(CachedAttackAbilityClass))
	{
		return;
	}
	
	if (UBlackboardComponent* BBComp = CachedOwnerComp->GetBlackboardComponent())
	{
		BBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
	}

	ANSEnemyAIController* AIController = Cast<ANSEnemyAIController>(CachedOwnerComp->GetAIOwner());
	if (AIController)
	{
		if (APawn* TargetPawn = AIController->GetPawn())
		{
			if (ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(TargetPawn))
			{
				Enemy->ClearCurrentAttackDefinition();
			}
			
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn))
			{
				ASC->OnAbilityEnded.RemoveAll(this);
			}
		}

		const bool bPreserveMeleeReservation = ShouldPreserveMeleeReservation(AIController);

		if (bUsesMeleeReservation && !bPreserveMeleeReservation)
		{
			AIController->ReleaseMeleeAttackReservation(true);
		}

		bUsesMeleeReservation = false;
	}
	
	CachedAttackAbilityClass = nullptr;

	// 애니메이션 종료
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

bool UNSBTTask_ExecuteEnemyAbility::ShouldPreserveMeleeReservation(const ANSEnemyAIController* AIController) const
{
	if (!bUsesMeleeReservation || !AIController)
	{
		return false;
	}

	const ANSEnemyCharacterBase* Enemy = Cast<ANSEnemyCharacterBase>(AIController->GetPawn());

	return Enemy && Enemy->IsHitReacting() && AIController->HasMeleeAttackReservation();
}
