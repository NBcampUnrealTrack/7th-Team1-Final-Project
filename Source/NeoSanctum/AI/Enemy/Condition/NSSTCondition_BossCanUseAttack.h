// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "NSSTCondition_BossCanUseAttack.generated.h"

class ANSBossAIController;

USTRUCT()
struct FNSSTCondition_BossCanUseAttackInstanceData
{
	GENERATED_BODY()

	// 사용 가능 여부를 검사할 AttackId
	UPROPERTY(EditAnywhere, Category = "Config")
	FName AttackId = NAME_None;
};

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.05
 * 
 * 클래스 개요 : Boss StateTree에서 지정 AttackId가 현재 조건에서 사용 가능한지 검사하는 Condition
*/
USTRUCT(meta = (DisplayName = "Boss Can Use Attack", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTCondition_BossCanUseAttack : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNSSTCondition_BossCanUseAttackInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	// StateTree Context에서 BossAIController를 찾는 함수
	ANSBossAIController* ResolveBossController(FStateTreeExecutionContext& Context) const;
};
