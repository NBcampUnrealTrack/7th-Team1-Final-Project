// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "NSBTService_ComputeRoamPoint.generated.h"

class ANSBossArenaBounds;

UCLASS()
class NEOSANCTUM_API UNSBTService_ComputeRoamPoint : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTService_ComputeRoamPoint();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	// 앵커 기준 새 랜덤 순찰 목표점을 산출해 MoveTarget에 기록
	void RollNewRoamPoint(UBehaviorTreeComponent& OwnerComp);

	// 배치된 ArenaBounds를 탐색해 캐시 (없으면 nullptr 유지)
	ANSBossArenaBounds* FindArenaBounds(UBehaviorTreeComponent& OwnerComp);

private:
	// 순찰 반경 (앵커 기준)
	UPROPERTY(EditAnywhere, Category = "Roam", meta = (ClampMin = "0.0"))
	float RoamRadius = 1500.f;

	// 목표점 리롤 주기(초)
	UPROPERTY(EditAnywhere, Category = "Roam", meta = (ClampMin = "0.1"))
	float RoamRerollInterval = 4.f;

	// 도착 판정 반경 (XY)
	UPROPERTY(EditAnywhere, Category = "Roam", meta = (ClampMin = "0.0"))
	float ArrivalThreshold = 200.f;

private:
	// ── 런타임 상태 (bCreateNodeInstance=true → 드론 개체별 독립) ──
	FVector AnchorLocation = FVector::ZeroVector;
	FVector CurrentRoamPoint = FVector::ZeroVector;
	float RerollTimer = 0.f;
	TWeakObjectPtr<ANSBossArenaBounds> CachedArenaBounds;
	bool bArenaBoundsSearched = false;
};