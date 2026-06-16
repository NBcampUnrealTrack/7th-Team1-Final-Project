// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_ExecuteEnemyAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyAIController.h"

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

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn);
	if (!ASC) return EBTNodeResult::Failed;

	TSubclassOf<UGameplayAbility> AttackAbilityClass = AIController->GetAttackAbilityClassByDistance();
	if (!AttackAbilityClass)
	{
		return EBTNodeResult::Failed;
	}
	
	CachedOwnerComp = &OwnerComp;
	
	ASC->OnAbilityEnded.AddUObject(this, &UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded);

	UE_LOG(LogTemp, Log, TEXT("[AI Attack] 현재 거리 기준 선택된 태그: %s"), *AttackAbilityClass->GetName());

	bool bActivated = ASC->TryActivateAbilityByClass(AttackAbilityClass);
	if (!bActivated)
	{
		ASC->OnAbilityEnded.RemoveAll(this);
		return EBTNodeResult::Failed;
	}
	
	if (UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent())
	{
		BBComp->SetValueAsBool(TEXT("bIsAttacking"), true);
	}
	
	// 애니메이션이 끝날 때까지 대기
	return EBTNodeResult::InProgress;
}

void UNSBTTask_ExecuteEnemyAbility::OnAttackAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!CachedOwnerComp) return;
	
	if (UBlackboardComponent* BBComp = CachedOwnerComp->GetBlackboardComponent())
	{
		BBComp->SetValueAsBool(TEXT("bIsAttacking"), false);
	}

	ANSEnemyAIController* AIController = Cast<ANSEnemyAIController>(CachedOwnerComp->GetAIOwner());
	if (AIController)
	{
		if (APawn* TargetPawn = AIController->GetPawn())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn))
			{
				ASC->OnAbilityEnded.RemoveAll(this);
			}
		}
	}

	// 애니메이션 종료
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
}
