// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "NSFlyingLocomotionComponent.generated.h"

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSFlyingLocomotionComponent : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:
	UNSFlyingLocomotionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

public:
	// @민재 : 로코모션 내부 상태 리셋 (텔포/풀 재활용 시 호출). Velocity, 고도 스무딩, 후퇴 상태 모두 초기화
	void ResetLocomotionState();
	
	// 사망 시 호출: 고도 유지/스티어링을 끄고 천천히 추락시킨다
	void StartDeathFall();

private:
	UPROPERTY(EditAnywhere, Category="Locomotion|DeathFall", meta=(ClampMin="0.0"))
	float DeathFallGravity = 500.f;        // 추락 가속(cm/s^2)

	UPROPERTY(EditAnywhere, Category="Locomotion|DeathFall", meta=(ClampMin="0.0"))
	float DeathFallMaxSpeed = 300.f;       // 최대 낙하 속도(cm/s, 낮을수록 천천히)

	UPROPERTY(EditAnywhere, Category="Locomotion|DeathFall", meta=(ClampMin="0.0"))
	float DeathFallHorizontalDamp = 1.5f;  // 수평 속도 감쇠(클수록 빨리 멈춤)

	bool bDeathFalling = false;
	float CachedDeceleration = 0.f;

#pragma region PublicAPI
public:
	// @민재 : BT Task에서 호출하는 이동 요청. 스티어링/회피/후퇴 판정 후 오너 폰의 AddMovementInput으로 반영
	void RequestMoveTowards(
		const FVector& TargetLocation);              // 목표 지점 (XY 평면)

	// @민재 : 회전 타겟 세팅. nullptr이면 velocity 방향으로 회전 복귀
	void SetRotationTarget(
		AActor* InTarget);                           // 회전 대상 (nullptr 허용)

	FORCEINLINE AActor* GetRotationTarget() const { return RotationTarget.Get(); }

	FORCEINLINE bool IsRetreating() const { return bIsRetreating; }
	
	// @민재 : 목표 지점 도착 여부를 XY 거리로 판정
	bool HasReachedLocation(
		const FVector& TargetLocation) const;        // 판정 기준 위치
#pragma endregion

#pragma region Rotation
protected:
	// @민재 : 오너 폰의 회전 갱신. 타겟이 있으면 타겟 방향, 없으면 자기 Velocity 방향으로 보간
	void UpdateRotation(float DeltaSeconds);

	UPROPERTY(EditAnywhere, Category="Locomotion|Rotation")
	float YawInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category="Locomotion|Rotation")
	float CombatYawInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category="Locomotion|Rotation")
	float MinSpeedToRotate = 50.f;

	// @민재 : 전투 회전 시 타겟과 최소 XY 거리 (이보다 가까우면 회전 스킵)
	UPROPERTY(EditAnywhere, Category="Locomotion|Rotation")
	float MinCombatDistanceToRotate = 50.f;
#pragma endregion

#pragma region Altitude
public:
	float GetAltitude() const { return Altitude; }

	// @민재 : 유지 고도 오프셋을 직접 갱신. 보스 회피 서비스의 상승/하강 선택 시 사용
	void SetAltitude(float NewAltitude) { Altitude = NewAltitude; }
	
protected:
	// @민재 : 고도 유지 로직. 지형 샘플 → 목표 높이 스무딩 → Z 입력 적용
	void MaintainAltitude(float DeltaSeconds);

	// @민재 : 단일 지점 아래로 라인 트레이스
	bool TraceGroundAt(
		const FVector& WorldXY,                      // 트레이스 시작 위치 (XY 기준)
		float& OutZ) const;                          // 히트한 지형의 Z 값 출력

	// @민재 : 여러 샘플 포인트를 순회하여 가장 높은 지형 Z 반환 (플레이어 위로 이동 시 안전)
	bool SampleHighestGround(
		float& OutGroundZ) const;                    // 가장 높은 지형 Z 출력

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

	// @민재 : 지면 트레이스 채널 (Drone_Ground)
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude")
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_WorldStatic;

	// @민재 : 드론이 부드럽게 오를 수 있는 최대 경사각. 초과 시 수평 속도 낮춤
	UPROPERTY(EditAnywhere, Category="Locomotion|Altitude", meta=(ClampMin="0.0", ClampMax="80.0"))
	float MaxClimbAngle = 45.f;

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
		const FVector& DesiredDirection);            // 정규화된 목표 방향

	// @민재 : 각 스티어링 방향으로 스윕하여 장애물 위험도 계산 (프레임간 스무딩 포함)
	void BuildDangerMap();

	// @민재 : 위험도가 임계 이하이면서 관심도가 최대인 방향 선택
	FVector ChooseSteeringDirection() const;

	// @민재 : 앞쪽 지형 경사가 드론 등반 한계를 초과하면 수평 속도 스케일 계산 (0.3 ~ 1.0)
	float ComputeSlopeSpeedScale() const;

	// @민재 : 사방 위험 시 후퇴 방향 선택 (DangerMap 최저값 방향). 유효한 방향 없으면 ZeroVector
	FVector ChooseRetreatDirection() const;

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

	// @민재 : 사방이 벽일 때 후퇴 여부. false면 기존 동작(정지)
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	bool bEnableRetreat = true;

	// @민재 : 후퇴 시 이동 입력 강도 (전진 1.0 대비)
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RetreatInputScale = 0.7f;

	// @민재 : 후퇴 진입 후 최소 유지 시간 (떨림 방지)
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance", meta=(ClampMin="0.0"))
	float RetreatMinDuration = 0.3f;

	// @민재 : 회피 트레이스 채널 (Drone_Avoidance)
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance")
	TEnumAsByte<ECollisionChannel> AvoidanceChannel = ECC_GameTraceChannel4;

	// @민재 : DangerMap 반응 속도 (초당 보간 비율, 클수록 빠름)
	UPROPERTY(EditAnywhere, Category="Locomotion|Avoidance", meta=(ClampMin="1.0"))
	float DangerResponseRate = 10.f;

	bool bIsRetreating = false;
	float RetreatEnteredTime = 0.f;

	TArray<float> SmoothedDangerMap;
	TArray<FVector> SteeringDirections;
	TArray<float> InterestMap;
	TArray<float> DangerMap;
	
private:
	// @민재 : 회피 스윕에서 제외할 같은 팀/같은 클래스 폰들을 근접 반경 내에서 수집 (드론끼리 상호 교착 방지)
	void CollectAvoidanceIgnoredActors(const APawn* OwnerPawn, TArray<AActor*>& OutIgnored) const;
	
#pragma endregion

#pragma region CachedRefs
protected:
	// @민재 : 회전 대상. 오너 폰의 CurrentEnemy가 동기화됨
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> RotationTarget;
#pragma endregion

#pragma region ScriptedMove
public:
	// 평상시 카이팅/고도유지를 잠시 끄고, Dest를 향해 지정 속도·고도로 직선 비행 시작
	// 이미 진행 중이면 목적지/속도만 갱신 (레그 전환 시 재호출 용도)
	void BeginScriptedMove(const FVector& DestXY, float TargetAltitude, float Speed);

	// 스크립트 이동 종료. 캐시해둔 평상시 값 복원 + 상태 리셋 → 자연스럽게 평상시 고도로 복귀
	void EndScriptedMove();

	// 현재 ScriptedDestXY에 도달했는지 (XY: ArrivalRadius, Z: AltitudeDeadZone 기준)
	bool HasReachedScriptedDest() const;

protected:
	// bScriptedMove 중 매 프레임 Dest를 향한 순수 수평 입력만 적용 (스티어링/회피/후퇴 미사용)
	void ApplyScriptedMoveInput(float DeltaSeconds);

private:
	bool bScriptedMove = false;
	FVector ScriptedDestXY = FVector::ZeroVector;

	// BeginScriptedMove 최초 진입 시 캐시, EndScriptedMove에서 복원
	float CachedAltitude = 0.f;
	float CachedMaxSpeed = 0.f;
	float CachedAcceleration = 0.f;
	float CachedMaxClimbSpeed = 0.f;
	TWeakObjectPtr<AActor> CachedRotationTarget;
#pragma endregion
	
#pragma region MovementLock
public:
	// @민재 : 스티어링 입력(RequestMoveTowards) 차단 여부 설정. 기본 false(차단 해제) → 드론 등 다른 사용자는 영향 없음
	void SetSteeringInputLocked(bool bLocked) { bSteeringInputLocked = bLocked; }

private:
	bool bSteeringInputLocked = false;
#pragma endregion
	
private:
	// @민재 : 서버 권한 체크. 오너 폰이 Authority일 때만 로직 실행
	bool HasAuthorityChecked() const;
};