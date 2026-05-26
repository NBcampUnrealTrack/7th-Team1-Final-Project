// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // FAIStimulus
#include "NSEnemyAIController.generated.h"

UCLASS()
class NEOSANCTUM_API ANSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANSEnemyAIController();

	/*
	 * 0번을 플레이어 진영
	 * 1번을 몬스터 진영으로 구성할 예정.
	 * AI Controller가 적을 필터링하기 위한 TeamId 조회
	 */
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

	// 타 진영을 보았을 때의 적대/우호 규칙 정의 수립
	virtual ETeamAttitude::Type GetTeamAttitudeTo(const AActor& Other) const;

protected:
	// 빙의 시점에 에디터에서 할당된 BT 가동
	virtual void OnPossess(APawn* InPawn) override;

	// 시각 센서가 갱신될 때 블랙보드로 데이터를 전달할 콜백 함수
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	// 시야/청각 설정 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	// 에디터에서 몬스터별로 다른 BT를 꽂을 수 있도록 노출
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Config")
	FName TargetActor = TEXT("TargetActor");
};
