// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"


ANSEnemyAIController::ANSEnemyAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ANSEnemyAIController::OnTargetPerceptionUpdated);
}

ETeamAttitude::Type ANSEnemyAIController::GetTeamAttitudeTo(const AActor& Other) const
{
	// 센서에 포착된 대상이 팀 인터페이스 마크를 가지고 있는지 확인
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(&Other))
	{
		// 0번(플레이어) 진영은 무조건 적으로 규정
		if (TeamAgent->GetGenericTeamId() == FGenericTeamId(0))
		{
			return ETeamAttitude::Type::Hostile;
		}
		// 1번(몬스터) 진영은 아군으로 규정
		else if (TeamAgent->GetGenericTeamId() == FGenericTeamId(1))
		{
			return ETeamAttitude::Type::Friendly;
		}
	}

	// 인터페이스가 없거나 그 외의 대상은 중립 처리
	return ETeamAttitude::Type::Neutral;
}

void ANSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ANSEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp) return;

	// 감지된 대상이 플레이어인지 재검증
	if (GetTeamAttitudeTo(*Actor) == ETeamAttitude::Type::Hostile)
	{
		// 시야에 적이 들어왔으면 주소 저장, 시야에서 완전히 놓쳤으면 nullptr 처리
		AActor* Target = Stimulus.WasSuccessfullySensed() ? Actor : nullptr;

		// 블랙보드 TargetActor 키에 실시간 업데이트
		BBComp->SetValueAsObject(TargetActor, Target);
	}
}
