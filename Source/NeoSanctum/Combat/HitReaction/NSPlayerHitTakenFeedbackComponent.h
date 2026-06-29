// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSPlayerHitTakenFeedbackComponent.generated.h"

// 플레이어가 피해를 받았을 때 로컬 피격 피드백을 처리하는 컴포넌트
UCLASS()
class NEOSANCTUM_API UNSPlayerHitTakenFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerHitTakenFeedbackComponent();

	// 피격 Context를 기반으로 로컬 피격 피드백을 재생
	UFUNCTION(BlueprintCallable, Category = "HitFeedback")
	void HandleHitTakenFeedback(const FNSHitTakenFeedbackContext& Context);

private:
	// 
	bool ShouldPlayLocalFeedback() const;
};
