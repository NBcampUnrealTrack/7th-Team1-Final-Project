// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_FlyingExecuteAbility.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSBossAIController.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyDroneAIController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Character/Enemy/NSEnemyPawnBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

UNSBTTask_FlyingExecuteAbility::UNSBTTask_FlyingExecuteAbility()
{
	NodeName = "Flying Execute Ability";
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UNSBTTask_FlyingExecuteAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;
	
	ANSEnemyPawnBase* OwnerPawn = Cast<ANSEnemyPawnBase>(AIC->GetPawn());
	if (!OwnerPawn) return EBTNodeResult::Failed;
	
	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerPawn);
	if (!EnemyAgent) return EBTNodeResult::Failed;
	
	UAbilitySystemComponent* OwnerASC = OwnerPawn->GetAbilitySystemComponent();
	if (!OwnerASC) return EBTNodeResult::Failed;
	
	const FNSEnemyAttackRow* SelectedAttack = SelectAttackRow(AIC);
	if (!SelectedAttack) return EBTNodeResult::Failed;
	
	TSubclassOf<UGameplayAbility> CurrentAbilityClass = SelectedAttack->AbilityClass;
	if (!CurrentAbilityClass) return EBTNodeResult::Failed;
	
	EnemyAgent->SetCurrentAttackRow(*SelectedAttack);
	
	CachedOwnerComp = &OwnerComp;
	CachedAttackAbilityClass = CurrentAbilityClass;
	CachedRecoverTime = SelectedAttack->RecoverTime;
	
	OwnerASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAttackAbilityEnded);
	if (!OwnerASC->TryActivateAbilityByClass(CurrentAbilityClass))
	{
		EnemyAgent->ClearCurrentAttackRow();
		OwnerASC->OnAbilityEnded.RemoveAll(this);
		CachedAttackAbilityClass = nullptr;
		return EBTNodeResult::Failed;
	}
	
	NotifyAttackUsed(AIC, *SelectedAttack);
	
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsAttackingKey, true);
	}
	
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UNSBTTask_FlyingExecuteAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	OwnerComp.GetWorld()->GetTimerManager().ClearTimer(RecoverTimerHandle);
	
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AIC ? AIC->GetPawn() : nullptr;
	UAbilitySystemComponent* OwnerASC =
		Cast<ANSEnemyPawnBase>(OwnerPawn) ? Cast<ANSEnemyPawnBase>(OwnerPawn)->GetAbilitySystemComponent() : nullptr;
	
	if (OwnerASC)
	{
		OwnerASC->OnAbilityEnded.RemoveAll(this);
		if (CachedAttackAbilityClass)
		{
			if (const FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromClass(CachedAttackAbilityClass))
			{
				OwnerASC->CancelAbilityHandle(Spec->Handle);
			}
		}
	}
	
	INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerPawn);
	if (EnemyAgent)
	{
		EnemyAgent->ClearCurrentAttackRow();
	}
	
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsAttackingKey, false);
	}
	
	CachedAttackAbilityClass = nullptr;
	CachedOwnerComp = nullptr;
	
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UNSBTTask_FlyingExecuteAbility::OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!CachedOwnerComp.IsValid()) return;
	if (CachedAttackAbilityClass &&
	AbilityEndedData.AbilityThatEnded &&
	!AbilityEndedData.AbilityThatEnded->IsA(CachedAttackAbilityClass))
	{
		return;   // 다른 GA의 종료 이벤트면 무시
	}
		
	if (UBlackboardComponent* BB = CachedOwnerComp.Get()->GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsAttackingKey, false);
	}
	
	if (AAIController* AIC = CachedOwnerComp->GetAIOwner())
	{
		APawn* OwnerPawn = AIC->GetPawn();

		if (INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerPawn))
		{
			EnemyAgent->ClearCurrentAttackRow();
		}

		if (ANSEnemyPawnBase* EnemyPawn = Cast<ANSEnemyPawnBase>(OwnerPawn))
		{
			if (UAbilitySystemComponent* OwnerASC = EnemyPawn->GetAbilitySystemComponent())
			{
				OwnerASC->OnAbilityEnded.RemoveAll(this);
			}
		}
	}

	CachedAttackAbilityClass = nullptr; 
	
	if (CachedRecoverTime > 0.f)
	{
		if (UWorld* World = CachedOwnerComp->GetWorld())
		{
			World->GetTimerManager().SetTimer(
				RecoverTimerHandle,
				this,
				&ThisClass::FinishAfterRecover,
				CachedRecoverTime,
				false);
			return;
		}
	}

	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

void UNSBTTask_FlyingExecuteAbility::FinishAfterRecover()
{
	if (!CachedOwnerComp.IsValid()) return;
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}

const FNSEnemyAttackRow* UNSBTTask_FlyingExecuteAbility::SelectAttackRow(AAIController* AIController) const
{
	ANSBossAIController* BossAIC = Cast<ANSBossAIController>(AIController);
	if (BossAIC)
	{
		return BossAIC->GetAttackRowByDistance();
	}
	
	if (ANSEnemyDroneAIController* DroneAIC = Cast<ANSEnemyDroneAIController>(AIController))
	{
		return DroneAIC->GetAttackRowByDistance();
	}
	return nullptr;
}

void UNSBTTask_FlyingExecuteAbility::NotifyAttackUsed(AAIController* AIController,
	const FNSEnemyAttackRow& AttackRow) const
{
	ANSBossAIController* BossAIC = Cast<ANSBossAIController>(AIController);
	if (BossAIC)
	{
		BossAIC->RecordAttackUsed(AttackRow);
	}
	
	if (ANSEnemyDroneAIController* DroneAIC = Cast<ANSEnemyDroneAIController>(AIController))
	{
		DroneAIC->RecordAttackUsed(AttackRow);
	}
}
