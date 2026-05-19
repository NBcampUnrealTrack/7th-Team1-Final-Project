// Copyright 2026 One Team. All rights reserved.

#include "NSCharacterAnimInstance.h"

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

	UpdateMovementData();
}

void UNSCharacterAnimInstance::RefreshOwningCharacter()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CharacterMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
}

void UNSCharacterAnimInstance::UpdateMovementData()
{
	if (!OwnerCharacter)
	{
		Velocity = FVector::ZeroVector;
		LocalVelocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
		GroundSpeed = 0.f;
		bHasAcceleration = false;
		bShouldMove = false;
		bIsFalling = false;
		return;
	}

	if (!CharacterMovement)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();
	}

	Velocity = OwnerCharacter->GetVelocity();
	LocalVelocity = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();

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

	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;
}
