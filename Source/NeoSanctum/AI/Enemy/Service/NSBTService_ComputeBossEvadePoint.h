// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "NSBTService_ComputeBossEvadePoint.generated.h"

class ANSBossMotherShip;

/*
 * 클래스 개요 : MotherShip 전용 회피 지점 계산 서비스.
 * 좌/우/상승/하강 중 하나를 랜덤 선택해 보스 현재 위치 기준 오프셋을 산출하고,
 * ArenaBounds 안으로 클램프해 MoveTarget(Vector) 블랙보드 키에 기록한다.
 * 이동 실행은 기존 NSBTTask_FlyMoveTo가 담당한다.
*/

UCLASS()
class NEOSANCTUM_API UNSBTService_ComputeBossEvadePoint : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTService_ComputeBossEvadePoint();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	enum class EBossEvadeAxis : uint8
	{
		Left,
		Right,
		Up,
		Down
	};

	// 다음 회피 방향을 다시 뽑을 시점까지 남은 시간을 갱신하는 함수
	void UpdateRerollTimer(float DeltaSeconds);

	// 현재 AI가 조종하는 MotherShip을 반환하는 함수
	ANSBossMotherShip* GetBossMotherShip(UBehaviorTreeComponent& OwnerComp) const;

private:
	// 좌/우 회피 시 XY 이동 거리
	UPROPERTY(EditAnywhere, Category = "Evade", meta = (ClampMin = "0.0"))
	float HorizontalEvadeDistance = 1200.f;

	// 상승/하강 회피 시 고도 변화량
	UPROPERTY(EditAnywhere, Category = "Evade", meta = (ClampMin = "0.0"))
	float VerticalEvadeDelta = 600.f;

	// 회피 방향을 다시 뽑는 주기(초)
	UPROPERTY(EditAnywhere, Category = "Evade", meta = (ClampMin = "0.1"))
	float RerollInterval = 2.f;

private:
	// ── 런타임 상태 (bCreateNodeInstance=true → 보스 개체별 독립) ──
	EBossEvadeAxis CurrentAxis = EBossEvadeAxis::Left;
	float RerollTimer = 0.f;
	float BaseAltitude = 0.f;
	bool bHasBaseAltitude = false;
};
