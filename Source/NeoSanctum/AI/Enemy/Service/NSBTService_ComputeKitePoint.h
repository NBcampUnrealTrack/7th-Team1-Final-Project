// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "NSBTService_ComputeKitePoint.generated.h"

struct FNSEnemyAttackRow;

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSBTService_ComputeKitePoint : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UNSBTService_ComputeKitePoint();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	// Level 1: 랜덤 주기마다 선회 방향을 확률적으로 뒤집는다
	void UpdateStrafeDirection(float DeltaSeconds);

	// Level 3(선택): 로코모션이 후퇴(사방 막힘) 중이면 즉시 방향 반전
	void TryReactiveFlip(UBehaviorTreeComponent& OwnerComp);

	// Level 2: 노이즈가 반영된 최종 선회각(부호 포함, 도)과 링 반경
	float ComputeEffectiveStrafeAngle() const;
	float ComputeEffectiveDistance() const;
	
protected:
	// 타겟과 유지할 거리(링 반경)
	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0.0"))
	float DesiredDistance = 800.f;

	// 현재 방위에서 옆으로 둘 각도(선회/회피 강도, 도)
	UPROPERTY(EditAnywhere, Category = "Kite")
	float StrafeOffsetAngle = 30.f;

	// 선회 방향 (+1 시계 / -1 반시계)
	UPROPERTY(EditAnywhere, Category = "Kite")
	float StrafeDirection = 1.f;

	// 타겟 액터를 읽어올 블랙보드 키 (Object). 출력 MoveTarget은 부모의 BlackboardKey(Vector) 사용
	UPROPERTY(EditAnywhere, Category = "Kite")
	FBlackboardKeySelector TargetActorKey;
	
	// ── Level 1: 방향 재추첨 ──
	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0"))
	float RerollIntervalMin = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0"))
	float RerollIntervalMax = 4.0f;

	// 재추첨 시 방향을 뒤집을 확률 (0=계속 유지, 1=매번 반전)
	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionFlipChance = 0.5f;

	// ── Level 2: 부드러운 노이즈 ──
	// 선회각 흔들림 폭(도). PerlinNoise 결과(약 ±1)에 곱해진다
	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0"))
	float AngleNoiseAmplitude = 20.f;

	// 링 반경 흔들림 폭. 접근/이탈 위빙 강도
	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0"))
	float DistanceNoiseAmplitude = 150.f;

	// 노이즈 진행 속도(작을수록 완만한 위빙)
	UPROPERTY(EditAnywhere, Category = "Kite|Random", meta = (ClampMin = "0.0"))
	float NoiseFrequency = 0.3f;

	// ── Level 3(선택): 막히면 반대로 ──
	UPROPERTY(EditAnywhere, Category = "Kite|Random")
	bool bReactiveFlipOnRetreat = true;

	
	
private:
	// ── 런타임 상태 (bCreateNodeInstance=true → 드론 개체별 독립) ──
	float CurrentStrafeDir = 1.f;   // 현재 선회 방향(±1)
	float RerollTimer = 0.f;        // 다음 재추첨까지 남은 시간(초)
	float TimeAccum = 0.f;          // 노이즈 입력용 누적 시간
	float NoisePhase = 0.f;         // 개체별 노이즈 위상 시드
	bool  bWasRetreating = false;   // Level 3 후퇴 진입 엣지 감지용
};
