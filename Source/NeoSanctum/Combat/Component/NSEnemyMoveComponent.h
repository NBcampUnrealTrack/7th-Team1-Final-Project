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

#pragma region NavMesh 밖 혹은 끼었을 때 텔레포트

public:
	// NavMesh 이탈 또는 이동 정체 상태를 감시하고 필요 시 복구 텔레포트를 수행하는 함수
	void UpdateNavigationRecovery(AAIController* Controller, float DeltaTime);

	// NavMesh 복구 감시 상태를 초기화하는 함수
	void ResetNavigationRecovery();

protected:
	// NavMesh 복구 텔레포트를 수행하기 전까지 대기할 시간을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float NavRecoveryDelay = 5.0f;

	// 몬스터가 이동했다고 판단할 최소 2D 이동 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float StuckMoveTolerance = 25.0f;

	// 몬스터가 거의 정지했다고 판단할 최대 2D 속도를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float StuckVelocityTolerance = 10.0f;

	// 현재 위치가 NavMesh 투영 위치에서 벗어났다고 판단할 허용 거리를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float NavOutsideTolerance = 150.0f;

	// 복구 텔레포트 위치를 탐색할 반경을 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	float RecoverySearchRadius = 700.0f;

	// 현재 위치를 NavMesh에 투영할 때 사용할 탐색 범위를 나타내는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Move|Recovery")
	FVector NavProjectionExtent = FVector(300.0f, 300.0f, 500.0f);

private:
	// NavMesh 이탈 또는 이동 정체 상태가 지속된 시간을 누적하는 변수
	float NavRecoveryElapsed = 0.0f;

	// 마지막 감시 위치가 기록되어 있는지 나타내는 변수
	bool bHasLastObservedLocation = false;

	// 이동 정체 여부를 판단하기 위해 마지막으로 기록한 위치를 나타내는 변수
	FVector LastObservedLocation = FVector::ZeroVector;

	// 마지막 유효 NavMesh 위치가 기록되어 있는지 나타내는 변수
	bool bHasLastValidNavLocation = false;

	// 복구 위치 탐색 기준으로 사용할 마지막 유효 NavMesh 위치를 나타내는 변수
	FVector LastValidNavLocation = FVector::ZeroVector;
#pragma endregion
};
