// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_ExecuteEnemyAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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

	FGameplayTag AttackTag = AIController->GetAttackAbilityTagByDistance();
	if (!AttackTag.IsValid()) return EBTNodeResult::Failed;

	UE_LOG(LogTemp, Log, TEXT("[AI Attack] 현재 거리 기준 선택된 태그: %s"), *AttackTag.ToString());

	// 추출된 태그를 컨테이너에 담아 몬스터 ASC에게 발동 명령 하달
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag));

	return EBTNodeResult::Succeeded;
}
