// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "NSTitanWalkerMoveComponent.generated.h"

struct FNSEnemyAttackRow;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.06
 *
 * 클래스 개요 : Stage1 TitanWalker의 지상 이동과 몸체 회전을 처리하는 MovementComponent
 * StateTree Task가 MobileMode 동안 호출하며, Pawn Velocity를 만들어 AnimBP Walk 값을 갱신
*/
UCLASS(ClassGroup = (Movement), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSTitanWalkerMoveComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UNSTitanWalkerMoveComponent();

	virtual void BeginPlay() override;

	// 이동 기준 타깃을 설정하는 함수
	void SetMoveTarget(AActor* InTargetActor);

	// 이동 기준 타깃을 제거하고 속도를 초기화하는 함수
	void StopMove();

	// StateTree Task에서 호출해 이동과 회전을 갱신하는 함수
	void TickMove(float DeltaSeconds);

	// 현재 이동 속도를 반환하는 함수
	float GetCurrentMoveSpeed() const { return Velocity.Size2D(); }

	// 현재 이동 중인지 확인하는 함수
	bool IsMoving() const { return Velocity.SizeSquared2D() > KINDA_SMALL_NUMBER; }

protected:
	// 이동에 사용할 UpdatedComponent를 루트 컴포넌트로 초기화하는 함수
	void InitializeUpdatedComponent();

	// 현재 TitanWalker가 이동/회전을 갱신할 수 있는 상태인지 확인하는 함수
	bool CanUpdateMovement() const;

	// 현재 공격 Row 기준으로 몸체 이동이 가능한지 확인하는 함수
	bool CanMoveBody() const;

	// 현재 공격 Row 기준으로 몸체 회전이 가능한지 확인하는 함수
	bool CanTurnBody() const;

	// 현재 공격 Row를 반환하는 함수
	const FNSEnemyAttackRow* GetCurrentAttackRow() const;

	// 타깃 방향으로 몸체 회전을 갱신하는 함수
	void UpdateBodyRotation(const FVector& DirectionToTarget, float DeltaSeconds);

	// 타깃과의 거리 기준으로 이동 속도를 계산하는 함수
	FVector CalculateMoveVelocity(const FVector& DirectionToTarget, float DistanceToTarget) const;

	// 이동 속도를 0으로 초기화하는 함수
	void ClearMoveVelocity();

protected:
	// TitanWalker 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TitanWalker|Move", meta = (ClampMin = "0.0"))
	float MoveSpeed = 220.0f;

	// TitanWalker 몸체 회전 속도. 단위는 degree/sec
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TitanWalker|Move", meta = (ClampMin = "0.0"))
	float TurnSpeed = 60.0f;

	// 타깃과 유지하려는 기준 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TitanWalker|Move", meta = (ClampMin = "0.0"))
	float DesiredDistance = 2500.0f;

	// 기준 거리 주변에서 이동을 멈추는 허용 오차
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TitanWalker|Move", meta = (ClampMin = "0.0"))
	float DistanceTolerance = 250.0f;

	// 타깃이 너무 가까울 때 후진 이동을 허용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TitanWalker|Move")
	bool bAllowBackwardMove = false;

private:
	// 현재 이동 기준 타깃
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> MoveTargetActor;
};
