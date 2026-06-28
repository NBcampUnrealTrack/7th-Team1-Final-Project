// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionTypes.h"
#include "NSHitReactionComponent.generated.h"

// Health Damage를 받은 액터의 월드 피격 리액션을 재생하는 컴포넌트
UCLASS()
class NEOSANCTUM_API UNSHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSHitReactionComponent();

	// Context를 기반으로 기본 GameplayCue를 실행
	UFUNCTION(BlueprintCallable, Category = "HitReaction")
	void PlayHitReaction(const FNSHitReactionContext& Context) const;

private:
	// Owner의 ASC에서 GameplayCue를 실행
	void ExecuteHitCue(const FNSHitReactionContext& Context) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DefaultHitCueTag;
};
