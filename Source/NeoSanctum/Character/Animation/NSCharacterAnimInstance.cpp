// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterAnimInstance.h"

#include "CharacterTrajectoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void UNSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// AnimInstance가 생성될 때 캐릭터와 필요한 컴포넌트들을 캐싱
	RefreshOwningCharacter();
}

void UNSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		RefreshOwningCharacter();
	}

	UpdateMovementData(DeltaSeconds);
}

void UNSCharacterAnimInstance::RefreshOwningCharacter()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CharacterMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	CharacterTrajectoryComponent = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UCharacterTrajectoryComponent>() : nullptr;
}

void UNSCharacterAnimInstance::SetSelectedAnimState(ENSAnimState NewSelectedAnimState)
{
	SelectedAnimState = NewSelectedAnimState;
}

void UNSCharacterAnimInstance::SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewSelectedPoseSearchDatabase)
{
	SelectedPoseSearchDatabase = NewSelectedPoseSearchDatabase;
}

void UNSCharacterAnimInstance::UpdateMovementData(float DeltaSeconds)
{
	if (!OwnerCharacter)
	{
		// 캐릭터가 없으면 ABP가 이전 프레임 값을 계속 쓰지 못하게 기본값으로 초기화
		Velocity = FVector::ZeroVector;
		LocalVelocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
		GroundSpeed = 0.f;
		VerticalVelocity = 0.f;
		bHasAcceleration = false;
		bShouldMove = false;
		bIsFalling = false;
		bWasFalling = false;
		bIsMovingUp = false;
		bHasLandRequest = false;
		bIsHeavyLand = false;
		PreviousGroundedAnimState = ENSAnimState::Idle;
		CurrentGroundedAnimState = ENSAnimState::Idle;
		AnimState = ENSAnimState::Idle;
		SelectedAnimState = ENSAnimState::Idle;
		SelectedPoseSearchDatabase = nullptr;
		LandVelocity = 0.f;
		PreviousVerticalVelocity = 0.f;
		return;
	}

	if (!CharacterMovement)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}

	Velocity = OwnerCharacter->GetVelocity();
	LocalVelocity = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	VerticalVelocity = Velocity.Z;
	bWasFalling = bIsFalling;

	// CharacterMovement에서 가속도와 공중 상태를 읽어 Locomotion 상태를 만듬
	if (CharacterMovement)
	{
		Acceleration = CharacterMovement->GetCurrentAcceleration();
		bHasAcceleration = Acceleration.SizeSquared2D() > UE_KINDA_SMALL_NUMBER;
		bIsFalling = CharacterMovement->IsFalling();
	}
	else
	{
		Acceleration = FVector::ZeroVector;
		bHasAcceleration = false;
		bIsFalling = false;
	}

	// 착지 순간에는 직전 프레임의 낙하 속도를 저장해서 Light/Heavy Land로 상태 구분
	bIsMovingUp = bIsFalling && VerticalVelocity > 0.f;
	if (bWasFalling && !bIsFalling)
	{
		LandVelocity = FMath::Abs(PreviousVerticalVelocity);
		bIsHeavyLand = LandVelocity >= HeavyLandVelocityThreshold;
		bHasLandRequest = true;
	}

	// 계산 순서가 꼬이면 안되는데, Aim/Turn 값을 먼저 만든 뒤에 AnimState에서 Turn 상태를 선택하도록 해야함
	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;
	UpdateAimData(DeltaSeconds);
	UpdateTurnInPlaceData();
	UpdateAnimState();
	PreviousVerticalVelocity = VerticalVelocity;
}

void UNSCharacterAnimInstance::UpdateAnimState()
{
	// 착지는 한 프레임짜리 요청으로 처리
	if (bHasLandRequest)
	{
		AnimState = bIsHeavyLand ? ENSAnimState::LandHeavy : ENSAnimState::LandLight;
		bHasLandRequest = false;
		return;
	}

	// 공중에서는 상승 중이면 JumpStart, 하강 중이면 FallLoop
	if (bIsFalling)
	{
		AnimState = bIsMovingUp ? ENSAnimState::JumpStart : ENSAnimState::FallLoop;
		return;
	}

	// 정지 중 카메라-몸 방향 차이가 크면 Turn In Place PSD에 따라 하체를 회전
	if (bShouldTurnInPlace)
	{
		AnimState = TurnInPlaceDirection == ENSTurnInPlaceDirection::Left
			? ENSAnimState::TurnInPlaceLeft
			: ENSAnimState::TurnInPlaceRight;
		return;
	}

	PreviousGroundedAnimState = CurrentGroundedAnimState;

	// 현재 속도에 따라 Idle/Walk/Run/Sprint 중 하나의 상태가 됨
	if (GroundSpeed < IdleSpeedThreshold)
	{
		CurrentGroundedAnimState = ENSAnimState::Idle;
	}
	else if (GroundSpeed < WalkSpeedThreshold)
	{
		CurrentGroundedAnimState = ENSAnimState::Walk;
	}
	else if (GroundSpeed < RunSpeedThreshold)
	{
		CurrentGroundedAnimState = ENSAnimState::Run;
	}
	else
	{
		CurrentGroundedAnimState = ENSAnimState::Sprint;
	}

	// 속도 단계가 바뀐 순간에는 전환 PSD에 따라 애니메이션 재생
	if (PreviousGroundedAnimState == ENSAnimState::Walk && CurrentGroundedAnimState == ENSAnimState::Run)
	{
		AnimState = ENSAnimState::WalkToRun;
	}
	else if (PreviousGroundedAnimState == ENSAnimState::Walk && CurrentGroundedAnimState == ENSAnimState::Sprint)
	{
		AnimState = ENSAnimState::WalkToSprint;
	}
	else if (PreviousGroundedAnimState == ENSAnimState::Run && CurrentGroundedAnimState == ENSAnimState::Walk)
	{
		AnimState = ENSAnimState::RunToWalk;
	}
	else if (PreviousGroundedAnimState == ENSAnimState::Run && CurrentGroundedAnimState == ENSAnimState::Sprint)
	{
		AnimState = ENSAnimState::RunToSprint;
	}
	else if (PreviousGroundedAnimState == ENSAnimState::Sprint && CurrentGroundedAnimState == ENSAnimState::Walk)
	{
		AnimState = ENSAnimState::SprintToWalk;
	}
	else if (PreviousGroundedAnimState == ENSAnimState::Sprint && CurrentGroundedAnimState == ENSAnimState::Run)
	{
		AnimState = ENSAnimState::SprintToRun;
	}
	else
	{
		AnimState = CurrentGroundedAnimState;
	}

	bIsHeavyLand = false;
}

void UNSCharacterAnimInstance::UpdateAimData(float DeltaSeconds)
{
	if (!OwnerCharacter)
	{
		TargetAimYaw = 0.f;
		AimYaw = 0.f;
		TargetAimPitch = 0.f;
		AimPitch = 0.f;
		AimOffsetAlpha = 0.f;
		return;
	}

	const FRotator AimRotation = GetViewRotation();
	const FRotator AimBaseRotation = GetAimBaseRotation();

	// Sample Project 방식에 가깝게 카메라 방향과 메쉬 root 기준 방향의 차이를 AimOffset 입력으로 사용
	TargetAimYaw = FRotator::NormalizeAxis(AimRotation.Yaw - AimBaseRotation.Yaw);
	TargetAimPitch = FRotator::NormalizeAxis(AimRotation.Pitch);

	// Yaw값의 경우 180/-180 경계에서 튀지 않도록 각도 차이를 먼저 구한 뒤에 보간
	const float InterpolationAlpha = FMath::Clamp(DeltaSeconds * AimInterpolationSpeed, 0.f, 1.f);
	const float YawDelta = FMath::FindDeltaAngleDegrees(AimYaw, TargetAimYaw);
	const float PitchDelta = FMath::FindDeltaAngleDegrees(AimPitch, TargetAimPitch);

	AimYaw = FRotator::NormalizeAxis(AimYaw + YawDelta * InterpolationAlpha);
	AimPitch = FRotator::NormalizeAxis(AimPitch + PitchDelta * InterpolationAlpha);
	AimOffsetAlpha = CharacterAnimType == ENSCharacterAnimType::Ranged
		? RangedAimOffsetAlpha
		: MeleeAimOffsetAlpha;
}

void UNSCharacterAnimInstance::UpdateTurnInPlaceData()
{
	if (!OwnerCharacter)
	{
		ViewYawDelta = 0.f;
		RootYawOffset = 0.f;
		bShouldTurnInPlace = false;
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		return;
	}

	const FRotator AimRotation = GetViewRotation();
	const FRotator AimBaseRotation = GetAimBaseRotation();

	// Turn In Place도 AimOffset과 같은 기준을 사용해야 상체/하체 회전에 문제가 생기지 않는다고 함
	ViewYawDelta = FRotator::NormalizeAxis(AimRotation.Yaw - AimBaseRotation.Yaw);
	RootYawOffset = ViewYawDelta;

	const float AbsYawDelta = FMath::Abs(ViewYawDelta);
	const bool bCanTurnInPlace = GroundSpeed < IdleSpeedThreshold && !bIsFalling;
	if (!bCanTurnInPlace)
	{
		bShouldTurnInPlace = false;
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		return;
	}
	
	bShouldTurnInPlace = bShouldTurnInPlace
		? AbsYawDelta > TurnInPlaceStopAngle
		: AbsYawDelta >= TurnInPlaceStartAngle;

	TurnInPlaceDirection = bShouldTurnInPlace
		? (ViewYawDelta < 0.f ? ENSTurnInPlaceDirection::Left : ENSTurnInPlaceDirection::Right)
		: ENSTurnInPlaceDirection::None;

	if (bShouldTurnInPlace)
	{
		// 하체가 상체를 따라 도는 중에는 상체 AimOffset의 영향을 조금 줄여서 허리 꼬임 현상을 줄이는 부분
		AimOffsetAlpha *= TurnInPlaceAimOffsetAlphaMultiplier;
	}
}

FRotator UNSCharacterAnimInstance::GetViewRotation() const
{
	if (!OwnerCharacter)
	{
		return FRotator::ZeroRotator;
	}

	if (const AController* Controller = OwnerCharacter->GetController())
	{
		return Controller->GetControlRotation();
	}

	return OwnerCharacter->GetBaseAimRotation();
}

FRotator UNSCharacterAnimInstance::GetAimBaseRotation() const
{
	if (!OwnerCharacter)
	{
		return FRotator::ZeroRotator;
	}

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (MeshComponent && MeshComponent->GetBoneIndex(AimBaseBoneName) != INDEX_NONE)
	{
		// Motion Matching/Turn In Place로 메쉬 root가 캡슐과 다르게 돌 수 있음. 따라서 root bone을 우선함.
		return MeshComponent->GetSocketTransform(AimBaseBoneName, RTS_World).Rotator();
	}

	return OwnerCharacter->GetActorRotation();
}
