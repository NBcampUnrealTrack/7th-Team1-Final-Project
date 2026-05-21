// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"

ANSPlayerCharacterBase::ANSPlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SocketOffset = FVector(0.0f, 50.0f, 0.0f);
	
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
	CameraComp->bUsePawnControlRotation = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->RotationRate = FRotator(0.f, 540.f, 0.f);

	InputBinderComp = CreateDefaultSubobject<UNSInputBinderComponent>(TEXT("InputBinderComp"));
	
	CharacterTrajectoryComp = CreateDefaultSubobject<UCharacterTrajectoryComponent> (TEXT("CharacterTrajectoryComp"));
}

void ANSPlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCameraFacingRotation(DeltaSeconds);
}

void ANSPlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ANSPlayerCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (InputBinderComp)
	{
		InputBinderComp->InitializePlayerInput(PlayerInputComponent);
	}
}

void ANSPlayerCharacterBase::UpdateCameraFacingRotation(float DeltaSeconds)
{
	if (!bUseCameraFacingRotation || !Controller)
	{
		bIsCameraFacingRotationActive = false;
		return;
	}

	const FRotator ActorRotation = GetActorRotation();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const float YawDelta = FRotator::NormalizeAxis(ControlRotation.Yaw - ActorRotation.Yaw);
	const float AbsYawDelta = FMath::Abs(YawDelta);
	const FVector HorizontalVelocity(GetVelocity().X, GetVelocity().Y, 0.f);
	const bool bShouldFaceCamera = HorizontalVelocity.SizeSquared() > FMath::Square(CameraFacingMoveSpeedThreshold);

	bIsCameraFacingRotationActive = bShouldFaceCamera
		|| (bIsCameraFacingRotationActive
			? AbsYawDelta > CameraFacingTurnStopAngle
			: AbsYawDelta >= CameraFacingTurnStartAngle);

	if (!bIsCameraFacingRotationActive)
	{
		return;
	}

	const FRotator TargetRotation(0.f, ControlRotation.Yaw, 0.f);
	const FRotator NewRotation = FMath::RInterpConstantTo(
		ActorRotation,
		TargetRotation,
		DeltaSeconds,
		CameraFacingRotationSpeed);

	SetActorRotation(NewRotation);
}
