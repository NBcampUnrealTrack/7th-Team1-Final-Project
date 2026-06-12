// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_CompanionActivateAbility.h"
#include "AIController.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"

UNSBTTask_CompanionActivateAbility::UNSBTTask_CompanionActivateAbility()
{
	NodeName = "Companion Activate Ability";
	bNotifyTick = true;
}

EBTNodeResult::Type UNSBTTask_CompanionActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                                                    uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	
	ANSBaseCompanionAI* CompanionAI = Cast<ANSBaseCompanionAI>(AIController->GetPawn());
	if (!CompanionAI) return EBTNodeResult::Failed;
	
	return EBTNodeResult::InProgress;
}

void UNSBTTask_CompanionActivateAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	ANSBaseCompanionAI* CompanionAI = Cast<ANSBaseCompanionAI>(AIController->GetPawn());
	if (!CompanionAI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	UAbilitySystemComponent* ASC = CompanionAI->GetAbilitySystemComponent();
	if (!ASC) return;
	
	ASC->TryActivateAbilitiesByTag(ActivateAbilityTags);
}
