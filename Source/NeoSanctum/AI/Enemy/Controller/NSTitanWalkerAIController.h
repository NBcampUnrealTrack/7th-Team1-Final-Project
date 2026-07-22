// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBossAIController.h"
#include "NSTitanWalkerAIController.generated.h"

class UNSBossAbilityExecutorComponent;

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Stage1 TitanWalker 보스의 StateTree 전투 판단을 담당하는 AI Controller
*/
UCLASS()
class NEOSANCTUM_API ANSTitanWalkerAIController : public ANSBossAIController
{
	GENERATED_BODY()

public:
	ANSTitanWalkerAIController();

	// TitanWalker StateTree 공격 실행 상태까지 포함해 공격 중인지 확인하는 함수
	virtual bool IsBossAttackInProgress() const override;

private:
	// 현재 Possess 중인 Pawn의 BossAbilityExecutorComponent를 반환하는 함수
	UNSBossAbilityExecutorComponent* GetBossAbilityExecutorComponent() const;
};
