// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSPlayerAttackFeedbackComponent.generated.h"

struct FNSPlayerAttackFeedbackData;

USTRUCT()
struct FNSPendingAttackFeedback
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FNSHitFeedbackContext Context;

	UPROPERTY(Transient)
	int32 Priority = TNumericLimits<int32>::Lowest();

	UPROPERTY(Transient)
	bool bHasValue = false;
};

// 플레이어가 공격으로 만든 실제 히트 결과를 UI 피드백으로 변환하는 컴포넌트
UCLASS()
class NEOSANCTUM_API UNSPlayerAttackFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSPlayerAttackFeedbackComponent();

	// 공격 히트 Context를 해석하고 DataTable 규칙에 맞는 피드백을 재생
	UFUNCTION(BlueprintCallable, Category = "HitFeedback")
	void HandleAttackHitFeedback(const FNSHitFeedbackContext& Context);

	// 서버에서 그룹 처리가 끝났다는 신호를 받으면 대표 피드백을 재생.
	void CompleteAttackHitFeedbackGroup(const FGuid& FeedbackGroupId);

private:
	// 같은 공격에서 연속으로 들어온 피드백 중 하나만 골라 재생.
	void PlayResolveAttackHitFeedback(
		const FNSHitFeedbackContext& Context,
		const FNSPlayerAttackFeedbackData& FeedbackData
	) const;

	// 더 중요한 결과가 들어왔는지 비교.
	bool ShouldReplacePendingFeedback(
		const FNSPendingAttackFeedback& PendingFeedback,
		const FNSHitFeedbackContext& CandidateContext,
		int32 CandidatePriority
	) const;

	UPROPERTY(Transient)
	TMap<FGuid, FNSPendingAttackFeedback> PendingAttackFeedbackGroups;

	// TargetType, Outcome을 채워 최종 매칭 Context를 구성
	FNSHitFeedbackContext BuildResolvedContext(const FNSHitFeedbackContext& Context) const;
	
	// 타겟 액터 클래스를 기준으로 피드백 대상 타입 판정
	ENSHitFeedbackTargetType ResolveTargetType(const AActor* TargetActor) const;
	
	// 타겟 고갈 여부를 기준으로 Kill/Destroy 결과 판정
	ENSHitFeedbackOutcome ResolveOutcome(const FNSHitFeedbackContext& Context) const;
	
	// DataTable에서 가장 우선순위가 높은 피드백 규칙 조회
	const FNSPlayerAttackFeedbackData* FindBestFeedbackData(const FNSHitFeedbackContext& Context) const;
	
	bool CheckFeedbackDataMatch(const FNSPlayerAttackFeedbackData& Data, const FNSHitFeedbackContext& Context) const;
	
	// GMS로 크로스헤어 피드백 메시지 전송
	void PlayCrosshairFeedback(ENSCrosshairAttackFeedbackType FeedbackType, const FNSHitFeedbackContext& Context) const;

	// 클라이언트 로컬에 AttackFeedback Sound를 재생
	void PlaySoundFeedback(FName SoundID) const;

};
