// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "UObject/Interface.h"
#include "NSHitReactionSourceInterface.generated.h"

// 월드 피격 리액션 분류를 제공하는 Damage Source 인터페이스
UINTERFACE(MinimalAPI)
class UNSHitReactionSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSHitReactionSourceInterface
{
	GENERATED_BODY()

public:
	// 이 Source의 공격 방식 분류
	virtual ENSHitReactionAttackType GetHitReactionAttackType() const = 0;
};
