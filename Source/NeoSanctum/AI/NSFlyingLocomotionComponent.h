// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSFlyingLocomotionComponent.generated.h"

class UFloatingMovementComponent;

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSFlyingLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSFlyingLocomotionComponent();

public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	
#pragma region PublicAPI
public:
	// @민재 : 오너 폰 컴포넌트 초기화
	void InitializeLocomotion(APawn* InOwnerPawn, UFloatingPawnMovement* InMovementComponent);
	
	// @민재 : BT Task 호출 이동 로직 
	void RequestMoveTowards(const FVector& TargetLocation);
	
	void SetRotationTarget(AActor* InTarget);
	
	FORCEINLINE AActor* GetRotationTarget() const { return RotationTarget.Get(); }
	
	bool HasReachedLocation(
		const FVector& TargetLocation);
	
#pragma endregion
	
#pragma region Altitude
protected:
	// @민재 : 고도 유지 로직. 지형 샘플 → 목표 높이 스무딩 → Z 입력 적용
	void MaintainAltitude(float DeltaSeconds);
	
	// @민재 : 단일 지점 아래로 라인 트레이스
	bool TraceGroundAt(const FVector& WorldXY, float& OutZ) const;
	
	// @민재 : 여러 샘플 포인트를 순회하여 가장 높은 지형 Z 반환 (플레이어 위로 이동 시 안전)
	bool SampleHighestGround(float& OutGroundZ) const;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float Altitude = 300.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float GroundTraceDistance = 2000.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float AltitudeDeadZone = 20.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float AltitudeCorrectionRange = 200.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float GroundSampleRadius = 150.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	int32 GroundSampleCount = 4;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float GroundLookAheadDistance = 200.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float GroundSampleInterval = 0.1f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float MaxClimbSpeed = 600.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	float MaxDescendSpeed = 150.f;
	
	float GroundSampleAccumulator = 0.f;
	float SmoothedTargetHeight = 0.f;
	bool bHasValidGround = false;
	
#pragma endregion
	
#pragma region Steering
protected:
	// @민재 : 회피 방향 벡터 배열 초기화 (BeginPlay에서 1회)
	void InitSteeringDirections();
	
	// @민재 : 목표 방향과 각 스티어링 방향의 관심도(내적) 계산
	void BuildInterestMap(
		const FVector& DesiredDirection);                // 원하는 이동 방향 (정규화됨)
	
	// @민재 : 각 스티어링 방향으로 스윕하여 장애물 위험도 계산
	void BuildDangerMap();
	
	// @민재 : 위험도가 임계 이하이면서 관심도가 최대인 방향 선택
	FVector ChooseSteeringDirection() const;
	
	// @민재 : 지면 각도 기준으로 걸을 수 있는 표면인지 판단 (걸을 수 있으면 위험도 무시)
	bool IsWalkableSurface(
		const FVector& SurfaceNormal) const;             // 히트된 표면 노말
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	float AvoidanceTraceDistance = 400.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	float AvoidanceTraceRadius = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	int32 NumSteeringDirections = 16;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	float DangerThreshold = 0.1f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	float MaxWalkableSlopeAngle = 50.f;
	
	UPROPERTY(EditAnywhere, Category="Locomotion|Movement")
	float ArrivalRadius = 60.f;
	
	TArray<FVector> SteeringDirections;
	TArray<float> InterestMap;
	TArray<float> DangerMap;
	
#pragma endregion
	
#pragma region CachedRefs
protected:
	// @민재 : 컴포넌트가 붙은 폰. InitializeLocomotion에서 세팅
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> OwnerPawn;
	
	// @민재 : 오너 폰의 이동 컴포넌트. 스티어링 결과가 반영될 대상
	UPROPERTY(Transient)
	TWeakObjectPtr<UFloatingPawnMovement> MovementComponent;
	
	// @민재 : 회전 대상. 오너 폰의 CurrentEnemy가 동기화됨
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> RotationTarget;
	
#pragma endregion

private:
	// @민재 : 서버 권한 체크. 오너 폰이 Authority일 때만 로직 실행
	bool HasAuthorityChecked() const;
};
