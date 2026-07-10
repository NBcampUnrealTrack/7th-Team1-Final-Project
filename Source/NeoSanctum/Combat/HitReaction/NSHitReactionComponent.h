// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionTypes.h"
#include "NSHitReactionComponent.generated.h"

struct FNSHitReactionData;

// Health Damage를 받은 액터의 월드 피격 리액션을 재생하는 컴포넌트
UCLASS()
class NEOSANCTUM_API UNSHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSHitReactionComponent();

	// Context를 기반으로 GameplayCue를 실행
	UFUNCTION(BlueprintCallable, Category = "HitReaction")
	void PlayHitReaction(const FNSHitReactionContext& Context) const;

	// 액터가 어떤 피격 대상인지 코드 생성 시 지정
	void SetTargetType(ENSHitFeedbackTargetType InTargetType) { TargetType = InTargetType; }

	ENSHitFeedbackTargetType GetTargetType() const { return TargetType; }

private:
	// 컴포넌트 설정값을 이용해 Context의 빈 분류 값을 보정
	FNSHitReactionContext BuildResolvedContext(const FNSHitReactionContext& Context) const;

	// 대상 사망/파괴 여부와 TargetType를 검사해서 히트 결과(Kill/Destroy) 결정
	ENSHitFeedbackOutcome ResolveOutcome(const FNSHitReactionContext& Context) const;

	// DataTable 매칭 결과를 우선 사용, 없으면 Default Cue Tag를 사용
	FGameplayTag ResolveCueTag(const FNSHitReactionContext& Context) const;
	
	// Context와 매칭되는 HitReaction Row를 Priority 기준으로 검색함
	const FNSHitReactionData* FindBestReactionData(const FNSHitReactionContext& Context) const;

	// Row의 조건이 Context와 매칭되는지 확인
	bool CheckReactionDataMatch(const FNSHitReactionData& Data, const FNSHitReactionContext& Context) const;

	// Owner의 ASC에서 GameplayCue를 실행
	void ExecuteHitCue(const FNSHitReactionContext& Context, FGameplayTag CueTag) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction", meta = (AllowPrivateAccess = "true"))
	ENSHitFeedbackTargetType TargetType = ENSHitFeedbackTargetType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction", meta = (AllowPrivateAccess = "true"))
	FGameplayTag DefaultHitCueTag;
};
