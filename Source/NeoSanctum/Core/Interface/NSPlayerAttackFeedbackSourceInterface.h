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
};
