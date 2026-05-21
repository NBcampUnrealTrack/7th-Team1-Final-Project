// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterAnimInstance.h"

#include "CharacterTrajectoryComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UNSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

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

void UNSCharacterAnimInstance::SetUpperBodyState(ENSUpperBodyState NewUpperBodyState)
{
	UpperBodyState = NewUpperBodyState;
}

void UNSCharacterAnimInstance::UpdateMovementData(float DeltaSeconds)
{
	if (!OwnerCharacter)
	{
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
		PreviousLocomotionState = ENSLocomotionState::Idle;
		LocomotionState = ENSLocomotionState::Idle;
		AirState = ENSAirState::Grounded;
		TurnInPlaceState = ENSTurnInPlaceState::None;
		bIsTurnInPlaceActive = false;
		TurnInPlaceYawDelta = 0.f;
		AnimState = ENSAnimState::Idle;
		SelectedAnimState = ENSAnimState::Idle;
		SelectedPoseSearchDatabase = nullptr;
		LandVelocity = 0.f;
		PreviousVerticalVelocity = 0.f;
		AimYaw = 0.f;
		AimPitch = 0.f;
		AimOffsetAlpha = 0.f;
		UpperBodyState = ENSUpperBodyState::None;
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

	// 착지 순간의 낙하 속도로 Light/Heavy Land를 구분
	bIsMovingUp = bIsFalling && VerticalVelocity > 0.f;
	if (bWasFalling && !bIsFalling)
	{
		LandVelocity = FMath::Abs(PreviousVerticalVelocity);
		bIsHeavyLand = LandVelocity >= HeavyLandVelocityThreshold;
		bHasLandRequest = true;
	}

	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;

	UpdateLocomotionState();
	UpdateAirState();
	UpdateTurnInPlaceState();
	UpdateAimData(DeltaSeconds);
	UpdateAnimState();

	PreviousVerticalVelocity = VerticalVelocity;
}

void UNSCharacterAnimInstance::UpdateLocomotionState()
{
	PreviousLocomotionState = LocomotionState;

	ENSLocomotionState CurrentSpeedState = ENSLocomotionState::Idle;
	if (GroundSpeed < IdleSpeedThreshold)
	{
		CurrentSpeedState = ENSLocomotionState::Idle;
	}
	else if (GroundSpeed < WalkSpeedThreshold)
	{
		CurrentSpeedState = ENSLocomotionState::Walk;
	}
	else if (GroundSpeed < RunSpeedThreshold)
	{
		CurrentSpeedState = ENSLocomotionState::Run;
	}
	else
	{
		CurrentSpeedState = ENSLocomotionState::Sprint;
	}

	// 속도의 단계가 바뀌는 순간에는 전환 상태로 변경
	if (PreviousLocomotionState == ENSLocomotionState::Walk && CurrentSpeedState == ENSLocomotionState::Run)
	{
		LocomotionState = ENSLocomotionState::WalkToRun;
	}
	else if (PreviousLocomotionState == ENSLocomotionState::Walk && CurrentSpeedState == ENSLocomotionState::Sprint)
	{
		LocomotionState = ENSLocomotionState::WalkToSprint;
	}
	else if (PreviousLocomotionState == ENSLocomotionState::Run && CurrentSpeedState == ENSLocomotionState::Walk)
	{
		LocomotionState = ENSLocomotionState::RunToWalk;
	}
	else if (PreviousLocomotionState == ENSLocomotionState::Run && CurrentSpeedState == ENSLocomotionState::Sprint)
	{
		LocomotionState = ENSLocomotionState::RunToSprint;
	}
	else if (PreviousLocomotionState == ENSLocomotionState::Sprint && CurrentSpeedState == ENSLocomotionState::Walk)
	{
		LocomotionState = ENSLocomotionState::SprintToWalk;
	}
	else if (PreviousLocomotionState == ENSLocomotionState::Sprint && CurrentSpeedState == ENSLocomotionState::Run)
	{
		LocomotionState = ENSLocomotionState::SprintToRun;
	}
	else
	{
		LocomotionState = CurrentSpeedState;
	}
}

void UNSCharacterAnimInstance::UpdateAirState()
{
	if (bHasLandRequest)
	{
		AirState = bIsHeavyLand ? ENSAirState::LandHeavy : ENSAirState::LandLight;
		bHasLandRequest = false;
		return;
	}

	if (bIsFalling)
	{
		AirState = bIsMovingUp ? ENSAirState::JumpStart : ENSAirState::FallLoop;
		return;
	}

	AirState = ENSAirState::Grounded;
	bIsHeavyLand = false;
}

void UNSCharacterAnimInstance::UpdateTurnInPlaceState()
{
	if (!OwnerCharacter || AirState != ENSAirState::Grounded || GroundSpeed >= IdleSpeedThreshold)
	{
		bIsTurnInPlaceActive = false;
		TurnInPlaceState = ENSTurnInPlaceState::None;
		TurnInPlaceYawDelta = 0.f;
		return;
	}

	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	TurnInPlaceYawDelta = FRotator::NormalizeAxis(AimRotation.Yaw - ActorRotation.Yaw);

	const float AbsYawDelta = FMath::Abs(TurnInPlaceYawDelta);
	bIsTurnInPlaceActive = bIsTurnInPlaceActive
		? AbsYawDelta > TurnInPlaceStopAngle
		: AbsYawDelta >= TurnInPlaceStartAngle;

	if (!bIsTurnInPlaceActive)
	{
		TurnInPlaceState = ENSTurnInPlaceState::None;
		return;
	}

	const bool bIsLeftTurn = TurnInPlaceYawDelta < 0.f;
	const bool bIs180Turn = AbsYawDelta >= TurnInPlace180Angle;

	if (bIsLeftTurn)
	{
		TurnInPlaceState = bIs180Turn ? ENSTurnInPlaceState::Left180 : ENSTurnInPlaceState::Left90;
	}
	else
	{
		TurnInPlaceState = bIs180Turn ? ENSTurnInPlaceState::Right180 : ENSTurnInPlaceState::Right90;
	}
}

void UNSCharacterAnimInstance::UpdateAimData(float DeltaSeconds)
{
	if (!OwnerCharacter)
	{
		AimYaw = 0.f;
		AimPitch = 0.f;
		AimOffsetAlpha = 0.f;
		return;
	}

	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	const FRotator AimDelta = (AimRotation - ActorRotation).GetNormalized();

	AimYaw = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Yaw), -AimYawLimit, AimYawLimit);
	AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Pitch), -AimPitchLimit, AimPitchLimit);

	const float TargetAlpha = UpperBodyState == ENSUpperBodyState::None ? 0.f : 1.f;
	AimOffsetAlpha = FMath::FInterpTo(AimOffsetAlpha, TargetAlpha, DeltaSeconds, AimOffsetBlendSpeed);
}

void UNSCharacterAnimInstance::UpdateAnimState()
{
	// 기존 Chooser Table용 상태 매핑
	if (AirState != ENSAirState::Grounded)
	{
		switch (AirState)
		{
		case ENSAirState::JumpStart:
			AnimState = ENSAnimState::JumpStart;
			break;
		case ENSAirState::FallLoop:
			AnimState = ENSAnimState::FallLoop;
			break;
		case ENSAirState::LandLight:
			AnimState = ENSAnimState::LandLight;
			break;
		case ENSAirState::LandHeavy:
			AnimState = ENSAnimState::LandHeavy;
			break;
		case ENSAirState::Grounded:
		default:
			AnimState = ENSAnimState::Idle;
			break;
		}
		return;
	}

	switch (LocomotionState)
	{
	case ENSLocomotionState::Idle:
		AnimState = ENSAnimState::Idle;
		break;
	case ENSLocomotionState::Walk:
		AnimState = ENSAnimState::Walk;
		break;
	case ENSLocomotionState::Run:
		AnimState = ENSAnimState::Run;
		break;
	case ENSLocomotionState::Sprint:
		AnimState = ENSAnimState::Sprint;
		break;
	case ENSLocomotionState::WalkToRun:
		AnimState = ENSAnimState::WalkToRun;
		break;
	case ENSLocomotionState::WalkToSprint:
		AnimState = ENSAnimState::WalkToSprint;
		break;
	case ENSLocomotionState::RunToWalk:
		AnimState = ENSAnimState::RunToWalk;
		break;
	case ENSLocomotionState::RunToSprint:
		AnimState = ENSAnimState::RunToSprint;
		break;
	case ENSLocomotionState::SprintToWalk:
		AnimState = ENSAnimState::SprintToWalk;
		break;
	case ENSLocomotionState::SprintToRun:
		AnimState = ENSAnimState::SprintToRun;
		break;
	default:
		AnimState = ENSAnimState::Idle;
		break;
	}
}
