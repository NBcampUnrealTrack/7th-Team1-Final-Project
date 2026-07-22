// NSSTCondition_BossCanUseMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"
#include "NSSTCondition_BossCanUseMode.generated.h"

class ANSBossAIController;

USTRUCT()
struct FNSSTCondition_BossCanUseModeInstanceData
{
	GENERATED_BODY()

	// 사용 가능한 공격이 있는지 검사할 Boss ModeTag
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Categories = "State.Enemy"))
	FGameplayTag ModeTag;
};

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.06
 * 
 * 클래스 개요 : Boss StateTree에서 지정 Boss ModeTag에 사용 가능한 공격이 있는지 검사하는 Condition
*/
USTRUCT(meta = (DisplayName = "Boss Can Use Mode", Category = "NeoSanctum|Boss"))
struct NEOSANCTUM_API FNSSTCondition_BossCanUseMode : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNSSTCondition_BossCanUseModeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	// StateTree Context에서 BossAIController를 찾는 함수
	ANSBossAIController* ResolveBossController(FStateTreeExecutionContext& Context) const;
};
