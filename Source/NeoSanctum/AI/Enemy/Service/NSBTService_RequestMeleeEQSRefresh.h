// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "NSBTService_RequestMeleeEQSRefresh.generated.h"

class UBehaviorTree;

UCLASS()
class NEOSANCTUM_API UNSBTService_RequestMeleeEQSRefresh : public UBTService
{
	GENERATED_BODY()

public:
	// Service 기본 실행 주기와 Blackboard 키 필터를 설정하는 생성자
	UNSBTService_RequestMeleeEQSRefresh();

protected:
	// Behavior Tree Blackboard에 선택한 키를 연결하는 함수
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	// Service가 활성화될 때 시야 상실 추적 상태를 초기화하는 함수
	virtual void OnBecomeRelevant(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	// 타깃 시야·거리·시야 상실 시간을 검사해서 EQS 정찰 필요 여부를 갱신하는 함수
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	// 현재 추적 중인 타깃 Actor를 읽는 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector TargetActorKey;

	// EQS 정찰 필요 여부를 저장하는 Blackboard Bool 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector NeedsRefreshKey;

	// AI Perception 기준으로 타깃 시야 확보 여부를 읽는 Blackboard Bool 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector HasLineOfSightKey;

	// 타깃이 이 거리 안에 있으면 시야가 잠깐 끊겨도 EQS 없이 직접 추적하는 거리
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS", meta = (ClampMin = "0.0"))
	float DirectChaseDistance = 900.0f;

	// 타깃 시야를 잃은 뒤 이 시간 이상 지나야 EQS 정찰을 요청하는 시간
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS", meta = (ClampMin = "0.0"))
	float LostSightSearchDelay = 3.0f;

	// 시야 상실 중 EQS 정찰 요청이 너무 자주 반복되지 않도록 막는 최소 간격
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS", meta = (ClampMin = "0.1"))
	float SearchRequestInterval = 2.0f;

private:
	// 현재 시야 상실 상태를 추적 중인지 여부
	bool bTrackingLostSight = false;

	// 시야를 잃기 시작한 월드 시간
	double LostSightStartTime = 0.0;

	// 마지막으로 EQS 정찰을 요청한 월드 시간
	double LastSearchRequestTime = -1000.0;
};
