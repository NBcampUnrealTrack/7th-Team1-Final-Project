// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSPlayerAttackFeedbackSourceInterface.generated.h"

// 플레이어 공격 결과 피드백을 발생시킬지 판단하는 Damage Source 인터페이스
UINTERFACE(MinimalAPI)
class UNSPlayerAttackFeedbackSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSPlayerAttackFeedbackSourceInterface
{
	GENERATED_BODY()

public:
	// 이 Source가 플레이어 공격 피드백을 발생시킬지 판단
	virtual bool ShouldTriggerPlayerAttackFeedback() const = 0;

	// 여러 대상에게 적용되는 한 번의 공격을 식별할 때 사용.
	// 단일 공격은 기본값을 그대로 사용.
	virtual FGuid GetPlayerAttackFeedbackGroupId() const { return FGuid(); }
};
