// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
#include "NSPlayerAttackFeedbackComponent.generated.h"

struct FNSPlayerAttackFeedbackData;

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

private:
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
