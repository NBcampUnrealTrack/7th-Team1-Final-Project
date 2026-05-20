// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NSCharacterAnimInstance.generated.h"

class ACharacter;
class UPoseSearchDatabase;
class UCharacterMovementComponent;
class UCharacterTrajectoryComponent;

UENUM(BlueprintType)
enum class ENSAnimState : uint8
{
	// 기본 이동속도 상태
	Idle,
	Walk,
	Run,
	Sprint,

	// 속도 단계가 바뀌는 순간의 상태
	WalkToRun,
	WalkToSprint,
	RunToWalk,
	RunToSprint,
	SprintToWalk,
	SprintToRun,

	// 제자리 회전 상태
	TurnInPlaceLeft,
	TurnInPlaceRight,

	// 공중/착지 상태
	JumpStart,
	FallLoop,
	LandLight,
	LandHeavy
};

UENUM(BlueprintType)
enum class ENSTurnInPlaceDirection : uint8
{
	None,
	Left,
	Right
};

UENUM(BlueprintType)
enum class ENSCharacterAnimType : uint8
{
	Melee,
	Ranged
};

UCLASS()
class NEOSANCTUM_API UNSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Animation|State", meta = (BlueprintThreadSafe))
	void SetSelectedAnimState(ENSAnimState NewSelectedAnimState);

	UFUNCTION(BlueprintCallable, Category = "Animation|State", meta = (BlueprintThreadSafe))
	void SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewSelectedPoseSearchDatabase);

protected:
	// 소유 캐릭터와 컴포넌트를 캐싱합니다. AnimGraph에서는 이 값을 읽기만 하는 구조를 유지
	void RefreshOwningCharacter();

	// 매 프레임 CharacterMovement 값을 애니메이션용 변수로 복사하고 파생 상태를 갱신
	void UpdateMovementData(float DeltaSeconds);

	// 현재 이동/공중/회전 조건을 ENSAnimState로 정리합니다. Chooser Table의 핵심 입력값
	void UpdateAnimState();

	// 카메라 방향과 메쉬 기준 방향의 차이를 AimOffset 입력값으로 계산
	void UpdateAimData(float DeltaSeconds);

	// 정지 중 카메라와 몸 방향 차이가 커졌는지 확인해 Turn In Place 상태를 계산
	void UpdateTurnInPlaceData();

	// 컨트롤러가 있으면 ControlRotation, 없으면 BaseAimRotation을 시선 방향으로 사용
	FRotator GetViewRotation() const;

	// AimOffset의 기준 방향
	FRotator GetAimBaseRotation() const;

protected:
	// 캐릭터 / 컴포넌트 캐싱
	// 매 프레임 Cast나 FindComponent를 반복하는 것이 비효율이라 초기화 시점에 캐싱
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Owner")
	TObjectPtr<UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	// 이동 입력 / 속도 값
	// CharacterMovement에서 복사한 원본 값과 ABP에서 쓸 수 있도록 변환한 값
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector LocalVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float VerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bWasFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsMovingUp = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bHasLandRequest = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	bool bIsHeavyLand = false;

	// Locomotion 상태
	// 속도 변화와 캐릭터 상태에 따른 Chooser Table을 통한 애니메이션 선택
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState PreviousGroundedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState CurrentGroundedAnimState = ENSAnimState::Idle;

	// Chooser Table이 읽고 애니메이션 데이터를 선택하게되는 상태
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState AnimState = ENSAnimState::Idle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	ENSAnimState SelectedAnimState = ENSAnimState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float LandVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float PreviousVerticalVelocity = 0.f;

	// 이동 상태 판정 기준값
	// 프로젝트의 실제 이동 속도와 애니메이션 풀에 맞춰 에디터에서 조정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float WalkSpeedThreshold = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float RunSpeedThreshold = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Movement")
	float HeavyLandVelocityThreshold = 900.f;

	// AimOffset
	// 캐릭터 타입에 따라 같은 AimOffset을 얼마나 강하게 적용할지 조절. 기본은 원거리 캐릭터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	ENSCharacterAnimType CharacterAnimType = ENSCharacterAnimType::Ranged;

	// AimOffset 기준이 되는 본. 일반적으로 root를 쓰는 경우가 많다고 하고 없으면 ActorRotation으로 대체하도록 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	FName AimBaseBoneName = TEXT("root");

	// 보간하기 전의 AimYaw
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float TargetAimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float TargetAimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float AimInterpolationSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float MeleeAimOffsetAlpha = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float RangedAimOffsetAlpha = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aim")
	float AimOffsetAlpha = 0.4f;

	// Turn In Place 중에는 하체가 도는 중이므로 상체 AimOffset 영향력을 어느 정도 줄여야 허리가 안 꼬임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Aim")
	float TurnInPlaceAimOffsetAlphaMultiplier = 0.5f;

	// Turn In Place
	// 정지 상태에서 카메라와 메쉬 기준 방향 차이가 커지면 하체를 회전하는 애니메이션
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float ViewYawDelta = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	bool bShouldTurnInPlace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Turn In Place")
	ENSTurnInPlaceDirection TurnInPlaceDirection = ENSTurnInPlaceDirection::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStartAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Turn In Place")
	float TurnInPlaceStopAngle = 10.f;
};
