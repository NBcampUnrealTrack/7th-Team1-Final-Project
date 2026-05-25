// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NSEnemyAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();

	/*
	 * 0번을 플레이어 진영
	 * 1번을 몬스터 진영으로 구성할 예정.
	 * AI Controller가 적을 필터링하기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

	// [책임] 타 진영을 보았을 때의 적대/우호 규칙 정의 수립
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

protected:
	// 시야/청각 설정 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;
};
