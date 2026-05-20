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
		AnimState = ENSAnimState::Grounded;
		SelectedAnimState = ENSAnimState::Grounded;
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

	bIsMovingUp = bIsFalling && VerticalVelocity > 0.f;
	if (bWasFalling && !bIsFalling)
	{
		LandVelocity = FMath::Abs(PreviousVerticalVelocity);
		bIsHeavyLand = LandVelocity >= HeavyLandVelocityThreshold;
		bHasLandRequest = true;
	}

	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;
	UpdateAnimState();
	PreviousVerticalVelocity = VerticalVelocity;
}

void UNSCharacterAnimInstance::UpdateAnimState()
{
	if (bHasLandRequest)
	{
		AnimState = bIsHeavyLand ? ENSAnimState::LandHeavy : ENSAnimState::LandLight;
		bHasLandRequest = false;
		return;
	}

	if (bIsFalling)
	{
		AnimState = bIsMovingUp ? ENSAnimState::JumpStart : ENSAnimState::FallLoop;
		return;
	}

	AnimState = Gait == ENSAnimGait::Sprint ? ENSAnimState::Sprint : ENSAnimState::Grounded;
	bIsHeavyLand = false;
}
