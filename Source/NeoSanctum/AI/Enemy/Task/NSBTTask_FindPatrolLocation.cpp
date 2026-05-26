// Copyright 2026 One Team. All rights reserved.


#include "NSBTTask_FindPatrolLocation.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UNSBTTask_FindPatrolLocation::UNSBTTask_FindPatrolLocation()
{
	NodeName = TEXT("Find Patrol Location");

	PatrolLocationKey.AddVectorFilter(this,
	                                  GET_MEMBER_NAME_CHECKED(UNSBTTask_FindPatrolLocation, PatrolLocationKey));
}

EBTNodeResult::Type UNSBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BBComp) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	// 언리얼 내장 네비게이션 시스템 추출
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem) return EBTNodeResult::Failed;

	// 현재 몬스터 위치 기준으로 PatrolRadius 반경 내의 갈 수 있는 무작위 좌표 추출
	FNavLocation RandomNavLocation;
	if (NavSystem->GetRandomReachablePointInRadius(AIPawn->GetActorLocation(),
	                                               PatrolRadius, RandomNavLocation))
	{
		// 무작위 좌표를 블랙보드의 Vector 키에 주입
		BBComp->SetValueAsVector(PatrolLocationKey.SelectedKeyName, RandomNavLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
