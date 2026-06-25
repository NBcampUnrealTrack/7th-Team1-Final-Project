// Copyright 2026 One Team. All rights reserved.

#include "NSBTTask_SetMeleeEQSRefresh.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UNSBTTask_SetMeleeEQSRefresh::UNSBTTask_SetMeleeEQSRefresh()
{
	NodeName = TEXT("Set Melee EQS Refresh");

	NeedsRefreshKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(
			UNSBTTask_SetMeleeEQSRefresh,
			NeedsRefreshKey));
}

void UNSBTTask_SetMeleeEQSRefresh::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		NeedsRefreshKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UNSBTTask_SetMeleeEQSRefresh::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!AIController ||
		!AIController->HasAuthority() ||
		!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsBool(NeedsRefreshKey.SelectedKeyName, bNeedsRefresh);

	return EBTNodeResult::Succeeded;
}
