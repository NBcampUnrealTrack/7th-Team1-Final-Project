// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

UENUM(BlueprintType)
enum class ENSAnimMovementMode : uint8
{
	OnGround,
	InAir
};

UENUM(BlueprintType)
enum class ENSAnimMovementState : uint8
{
	Idle,
	Moving
};

UENUM(BlueprintType)
enum class ENSAnimGait : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class ENSAnimCombatType : uint8
{
	None,
	Ranged,
	Melee
};

UENUM(BlueprintType)
enum class ENSTurnInPlaceDirection : uint8
{
	None,
	Left90,
	Right90,
	Left180,
	Right180
};

UCLASS()
class NEOSANCTUM_API UNSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 외부 로직에서도 전투타입을 갱신해야할 것 같아서 만든 세터
	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatType(ENSAnimCombatType NewCombatType);

protected:
	// AnimInstance 참조 캐릭터/컴포넌트 재설정
	void RefreshCachedReferences();

	// 런타임 값 초기화
	void ResetRuntimeData();

	// 매 프레임 변하는 속도, 가속도, 이동 방향같은 변수 값 갱신
	void UpdateMovementData();

	// 지상/공중 상태 갱신
	void UpdateMovementMode();

	// 정지/이동 상태 갱신
	void UpdateMovementState();

	// 이동 속도 기준 Walk/Run/Sprint 상태를 결정함
	void UpdateGait();

	// 이동 시작/정지 사이의 전환값 결정
	void UpdateStartStopData(float DeltaSeconds);

	// 착지 직후 Light/Heavy 착지 상태 유지를 통해 애니메이션일 잠시 더 유지하기 위한 갱신
	void UpdateLandingData(float DeltaSeconds);

	// 급격한 방향 전환 시 Pivot 상태로 갱신
	void UpdatePivotData(float DeltaSeconds);

	// 큰 이동 방향 변화에 따른 회전 계산으로 Transition 상태 갱신
	void UpdateSpinTransitionData();

	// 전투 타입별 상체 레이어 사용 여부 갱신 : 현재 None / Ranged / Melee 상태
	void UpdateCombatData(float DeltaSeconds);

	// 컨트롤 회전과 캐릭터 회전 차이 기반 조준 오프셋 계산
	void UpdateAimData();

	// 정지 상태의 조준 방향 기반 제자리 회전 계산
	void UpdateTurnInPlaceData();

	// 공중에서 지면까지의 예상 도달 시간 계산
	void UpdateTimeToLand();

protected:
	// 캐릭터, 캐릭터 무브먼트 컴포넌트 등 필요한 참조들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|References")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// 이동 속도와 방향관련 변수들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	FVector LocalAcceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float Speed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float PreviousSpeed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float StopSpeed2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float VerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float LocomotionAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float AccelerationAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bShouldMove = false;

	// Chooser 선택에 사용하는 상태 Enum 값들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementMode MovementMode = ENSAnimMovementMode::OnGround;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementState MovementState = ENSAnimMovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimMovementState PreviousMovementState = ENSAnimMovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait Gait = ENSAnimGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait PreviousGait = ENSAnimGait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	ENSAnimGait StopGait = ENSAnimGait::Walk;

	// Chooser 선택에 사용하는 전이 상태 bool 값들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bIsStarting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bIsPivoting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bJustLandedLight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bJustLandedHeavy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bShouldTurnInPlace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	bool bShouldSpinTransition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Chooser")
	float TimeToLand = 0.f;

	// 전투 타입에 따른 애니메이션 상태값들
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Combat")
	ENSAnimCombatType CombatType = ENSAnimCombatType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Combat")
	bool bUseUpperBodyLayer = false;

	// AimOffset용 조준 방향값
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	// 제자리 회전 상태를 판단하기 위한 값
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	ENSTurnInPlaceDirection TurnInPlaceDirection = ENSTurnInPlaceDirection::None;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceYawDelta = 0.f;

	// 이동 판정 기준값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float MoveSpeedThreshold = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	float HeavyLandSpeedThreshold = 900.f;

	// 자연스러운 애니메이션 재생을 위한 상태 유지 시간 (예를들면 Air -> Land 된 후 Land 상태를 얼마나 유지할 것인지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float LandStateHoldTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float StartStateHoldTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float PivotStateHoldTime = 0.2f;

	// 피벗과 회전 상태 전환 판정을 위한 값들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float PivotAccelerationDotThreshold = -0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SpinTransitionAngle = 135.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float TimeToLandTraceDistance = 5000.f;

	// 제자리 회전 판정 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStartAngle = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStopAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlace180Angle = 135.f;

	// 조준 각도 제한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AimYawLimit = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AimPitchLimit = 90.f;

private:
	bool bWasFalling = false;
	float PreviousVerticalVelocity = 0.f;
	float StartStateRemainingTime = 0.f;
	float LandStateRemainingTime = 0.f;
	float PivotStateRemainingTime = 0.f;
};
