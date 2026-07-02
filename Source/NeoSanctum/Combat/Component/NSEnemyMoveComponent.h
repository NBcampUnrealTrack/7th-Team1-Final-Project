// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyMoveComponent.generated.h"

class AAIController;
class ANSEnemyCharacterBase;
class UNSEnemyData;

struct FNSRetreatResult
{
	bool bShouldRetreat = false;
	bool bHasLocation = false;
	FVector Location = FVector::ZeroVector;
};

/**
 * 일반 Enemy의 후퇴 위치 계산과 전투 중 회전 방향 제어를 담당하는 컴포넌트입니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyMoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyMoveComponent();

	// 현재 타겟과 거리 기준으로 후퇴 여부와 후퇴 위치를 계산하는 함수
	FNSRetreatResult UpdateRetreat(
		AActor* TargetActor,
		bool bWasRetreating,
		bool bHasCurrentLocation,
		const FVector& CurrentLocation);

	// 후퇴 상태를 초기화하는 함수
	void ClearRetreat();

	// 공격/후퇴/준비 상태에 따라 이동 회전 또는 타겟 바라보기를 적용하는 함수
	void ApplyFacing(AAIController* Controller, AActor* TargetActor, AActor* AimActor, bool bFaceTarget);

	// 현재 거리에서 사용 가능한 공격이 있는지 확인하는 함수
	bool IsWithinAttackRange(AActor* TargetActor) const;

protected:
	// 후퇴 종료 거리 보정값. 경계에서 전진/후퇴가 반복되는 것을 줄임
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatExitBuffer = 100.0f;

	// 새 후퇴 지점을 계산할 때 기본으로 떨어질 거리
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatStepDistance = 250.0f;

	// 현재 후퇴 지점에 도착했다고 판단할 거리
	UPROPERTY(EditDefaultsOnly, Category = "Move|Retreat")
	float RetreatAcceptanceRadius = 75.0f;

private:
	ANSEnemyCharacterBase* GetOwnerEnemy() const;
	const UNSEnemyData* GetEnemyData() const;
	float GetMinAttackRange() const;
};
