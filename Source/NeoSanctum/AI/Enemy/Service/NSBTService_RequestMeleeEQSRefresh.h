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

	// Service가 활성화될 때 이전 타깃 위치 기록을 초기화하는 함수
	virtual void OnBecomeRelevant(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	// 타깃 이동 거리를 검사하고 필요하면 EQS 재탐색을 요청하는 함수
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	// 현재 추적 중인 타깃 Actor를 읽는 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector TargetActorKey;

	// EQS 재탐색 요청 상태를 읽고 기록하는 Blackboard 키
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS")
	FBlackboardKeySelector NeedsRefreshKey;

	// 이 거리 이상 타깃이 이동하면 기존 EQS 위치를 오래된 것으로 판단
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS", meta = (ClampMin = "1.0"))
	float TargetRequeryDistance = 500.0f;

	// 타깃 이동으로 EQS를 다시 요청할 수 있는 최소 간격
	UPROPERTY(EditAnywhere, Category = "AI|Melee EQS", meta = (ClampMin = "0.1"))
	float MinimumRequeryInterval = 2.0f;

private:
	// 마지막 EQS 결과가 결정됐을 때의 타깃 위치
	FVector QueryReferenceTargetLocation = FVector::ZeroVector;

	// 마지막 EQS 기준 위치를 기록한 월드 시간
	double QueryReferenceTime = 0.0;

	// 현재 유효한 EQS 기준 위치가 기록되어 있는지 여부
	bool bHasQueryReference = false;
};
