// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyAIController.h"

#include "Perception/AIPerceptionComponent.h"


ANSEnemyAIController::ANSEnemyAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
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
