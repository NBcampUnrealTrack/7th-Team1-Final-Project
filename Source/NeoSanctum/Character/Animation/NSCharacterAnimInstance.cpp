// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterAnimInstance.h"

#include "CharacterTrajectoryComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

void UNSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshCachedReferences();
}

void UNSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		RefreshCachedReferences();
	}

	if (!OwnerCharacter)
	{
		ResetRuntimeData();
		return;
	}

	PreviousMovementState = MovementState;
	PreviousGait = Gait;
	PreviousSpeed2D = Speed2D;

	UpdateMovementData();
	UpdateMovementMode();
	UpdateMovementState();
	UpdateGait();
	UpdateStartStopData(DeltaSeconds);
	UpdateLandingData(DeltaSeconds);
	UpdatePivotData(DeltaSeconds);
	UpdateSpinTransitionData();
	UpdateCombatData(DeltaSeconds);
	UpdateAimData();
	UpdateTurnInPlaceData();
	UpdateTimeToLand();

	PreviousVerticalVelocity = VerticalVelocity;
	bWasFalling = MovementMode == ENSAnimMovementMode::InAir;
}

void UNSCharacterAnimInstance::SetCombatType(ENSAnimCombatType NewCombatType)
{
	CombatType = NewCombatType;
}

void UNSCharacterAnimInstance::RefreshCachedReferences()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CharacterMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	CharacterTrajectoryComponent = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UCharacterTrajectoryComponent>() : nullptr;
}

void UNSCharacterAnimInstance::ResetRuntimeData()
{
	CharacterMovement = nullptr;
	CharacterTrajectoryComponent = nullptr;

	Velocity = FVector::ZeroVector;
	LocalVelocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	LocalAcceleration = FVector::ZeroVector;
	Speed2D = 0.f;
	GroundSpeed = 0.f;
	PreviousSpeed2D = 0.f;
	StopSpeed2D = 0.f;
	VerticalVelocity = 0.f;
	LocomotionAngle = 0.f;
	AccelerationAngle = 0.f;
	bHasAcceleration = false;
	bShouldMove = false;

	MovementMode = ENSAnimMovementMode::OnGround;
	MovementState = ENSAnimMovementState::Idle;
	PreviousMovementState = ENSAnimMovementState::Idle;
	Gait = ENSAnimGait::Walk;
	PreviousGait = ENSAnimGait::Walk;
	StopGait = ENSAnimGait::Walk;

	bIsStarting = false;
	bIsPivoting = false;
	bJustLandedLight = false;
	bJustLandedHeavy = false;
	bShouldTurnInPlace = false;
	bShouldSpinTransition = false;
	TimeToLand = 0.f;

	CombatType = ENSAnimCombatType::None;
	bUseUpperBodyLayer = false;

	AimYaw = 0.f;
	AimPitch = 0.f;

	TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
	TurnInPlaceYawDelta = 0.f;

	bWasFalling = false;
	PreviousVerticalVelocity = 0.f;
	StartStateRemainingTime = 0.f;
	LandStateRemainingTime = 0.f;
	PivotStateRemainingTime = 0.f;
}

void UNSCharacterAnimInstance::UpdateMovementData()
{
	if (!CharacterMovement)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}

	Velocity = OwnerCharacter->GetVelocity();
	LocalVelocity = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	Speed2D = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	GroundSpeed = Speed2D;
	VerticalVelocity = Velocity.Z;
	LocomotionAngle = Speed2D > MoveSpeedThreshold
		? FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X))
		: 0.f;

	if (CharacterMovement)
	{
		Acceleration = CharacterMovement->GetCurrentAcceleration();
		LocalAcceleration = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Acceleration);
		bHasAcceleration = Acceleration.SizeSquared2D() > UE_KINDA_SMALL_NUMBER;
		AccelerationAngle = bHasAcceleration
			? FMath::RadiansToDegrees(FMath::Atan2(LocalAcceleration.Y, LocalAcceleration.X))
			: 0.f;
	}
	else
	{
		Acceleration = FVector::ZeroVector;
		LocalAcceleration = FVector::ZeroVector;
		bHasAcceleration = false;
		AccelerationAngle = 0.f;
	}

	bShouldMove = Speed2D > MoveSpeedThreshold && bHasAcceleration;
}

void UNSCharacterAnimInstance::UpdateMovementMode()
{
	const bool bIsFalling = CharacterMovement && CharacterMovement->IsFalling();
	MovementMode = bIsFalling
		? ENSAnimMovementMode::InAir
		: ENSAnimMovementMode::OnGround;
}

void UNSCharacterAnimInstance::UpdateMovementState()
{
	MovementState = Speed2D > MoveSpeedThreshold
		? ENSAnimMovementState::Moving
		: ENSAnimMovementState::Idle;
}

void UNSCharacterAnimInstance::UpdateGait()
{
	if (MovementState == ENSAnimMovementState::Idle)
	{
		return;
	}

	if (Speed2D < WalkSpeedThreshold)
	{
		Gait = ENSAnimGait::Walk;
	}
	else if (Speed2D < RunSpeedThreshold)
	{
		Gait = ENSAnimGait::Run;
	}
	else
	{
		Gait = ENSAnimGait::Sprint;
	}
}

void UNSCharacterAnimInstance::UpdateStartStopData(float DeltaSeconds)
{
	const bool bStartedMoving =
		PreviousMovementState == ENSAnimMovementState::Idle &&
		MovementState == ENSAnimMovementState::Moving;

	const bool bStoppedMoving =
		PreviousMovementState == ENSAnimMovementState::Moving &&
		MovementState == ENSAnimMovementState::Idle;

	if (bStartedMoving)
	{
		StartStateRemainingTime = StartStateHoldTime;
	}
	else
	{
		StartStateRemainingTime = FMath::Max(0.f, StartStateRemainingTime - DeltaSeconds);
	}

	if (bStoppedMoving)
	{
		StopGait = PreviousGait;
		StopSpeed2D = PreviousSpeed2D;
	}

	bIsStarting = StartStateRemainingTime > 0.f;
}

void UNSCharacterAnimInstance::UpdateLandingData(float DeltaSeconds)
{
	const bool bIsInAir = MovementMode == ENSAnimMovementMode::InAir;
	const bool bJustLanded = bWasFalling && !bIsInAir;

	if (bJustLanded)
	{
		const float LandSpeed = FMath::Abs(PreviousVerticalVelocity);
		bJustLandedHeavy = LandSpeed >= HeavyLandSpeedThreshold;
		bJustLandedLight = !bJustLandedHeavy;
		LandStateRemainingTime = LandStateHoldTime;
	}
	else if (bIsInAir)
	{
		bJustLandedLight = false;
		bJustLandedHeavy = false;
		LandStateRemainingTime = 0.f;
	}
	else
	{
		LandStateRemainingTime = FMath::Max(0.f, LandStateRemainingTime - DeltaSeconds);
		if (LandStateRemainingTime <= 0.f)
		{
			bJustLandedLight = false;
			bJustLandedHeavy = false;
		}
	}
}

void UNSCharacterAnimInstance::UpdatePivotData(float DeltaSeconds)
{
	const FVector Velocity2D = FVector(Velocity.X, Velocity.Y, 0.f);
	const FVector Acceleration2D = FVector(Acceleration.X, Acceleration.Y, 0.f);
	const bool bWantsPivot =
		MovementState == ENSAnimMovementState::Moving &&
		bHasAcceleration &&
		Velocity2D.SizeSquared() > FMath::Square(MoveSpeedThreshold) &&
		FVector::DotProduct(Velocity2D.GetSafeNormal(), Acceleration2D.GetSafeNormal()) <= PivotAccelerationDotThreshold;

	if (bWantsPivot)
	{
		PivotStateRemainingTime = PivotStateHoldTime;
	}
	else
	{
		PivotStateRemainingTime = FMath::Max(0.f, PivotStateRemainingTime - DeltaSeconds);
	}

	bIsPivoting = PivotStateRemainingTime > 0.f;
}

void UNSCharacterAnimInstance::UpdateSpinTransitionData()
{
	bShouldSpinTransition =
		MovementMode == ENSAnimMovementMode::OnGround &&
		MovementState == ENSAnimMovementState::Moving &&
		Gait != ENSAnimGait::Sprint &&
		FMath::Abs(LocomotionAngle) >= SpinTransitionAngle;
}

void UNSCharacterAnimInstance::UpdateCombatData(float DeltaSeconds)
{
	bUseUpperBodyLayer = CombatType != ENSAnimCombatType::None;
}

void UNSCharacterAnimInstance::UpdateAimData()
{
	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	const FRotator AimDelta = (AimRotation - ActorRotation).GetNormalized();

	AimYaw = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Yaw), -AimYawLimit, AimYawLimit);
	AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimDelta.Pitch), -AimPitchLimit, AimPitchLimit);
}

void UNSCharacterAnimInstance::UpdateTurnInPlaceData()
{
	if (MovementMode != ENSAnimMovementMode::OnGround || MovementState != ENSAnimMovementState::Idle)
	{
		bShouldTurnInPlace = false;
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		TurnInPlaceYawDelta = 0.f;
		return;
	}

	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimRotation = OwnerCharacter->GetBaseAimRotation();
	TurnInPlaceYawDelta = FRotator::NormalizeAxis(AimRotation.Yaw - ActorRotation.Yaw);

	const float AbsYawDelta = FMath::Abs(TurnInPlaceYawDelta);
	bShouldTurnInPlace = bShouldTurnInPlace
		? AbsYawDelta > TurnInPlaceStopAngle
		: AbsYawDelta >= TurnInPlaceStartAngle;

	if (!bShouldTurnInPlace)
	{
		TurnInPlaceDirection = ENSTurnInPlaceDirection::None;
		return;
	}

	const bool bIsLeftTurn = TurnInPlaceYawDelta < 0.f;
	const bool bIs180Turn = AbsYawDelta >= TurnInPlace180Angle;

	if (bIsLeftTurn)
	{
		TurnInPlaceDirection = bIs180Turn ? ENSTurnInPlaceDirection::Left180 : ENSTurnInPlaceDirection::Left90;
	}
	else
	{
		TurnInPlaceDirection = bIs180Turn ? ENSTurnInPlaceDirection::Right180 : ENSTurnInPlaceDirection::Right90;
	}
}

void UNSCharacterAnimInstance::UpdateTimeToLand()
{
	TimeToLand = 0.f;

	if (MovementMode != ENSAnimMovementMode::InAir || VerticalVelocity >= 0.f || !OwnerCharacter)
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector TraceStart = OwnerCharacter->GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TimeToLandTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NSAnimInstanceTimeToLand), false, OwnerCharacter);
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return;
	}

	const float CollisionHalfHeight = OwnerCharacter->GetSimpleCollisionHalfHeight();
	const float DistanceToGround = FMath::Max(0.f, HitResult.Distance - CollisionHalfHeight);
	TimeToLand = DistanceToGround / FMath::Max(FMath::Abs(VerticalVelocity), 1.f);
}
