// Copyright 2026 One Team. All rights reserved.

#include "NSBaseCompanionAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

ANSBaseCompanionAI::ANSBaseCompanionAI()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// AIController 자동빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ANSDroneAIController::StaticClass();
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Collision");
	SetRootComponent(SphereComponent);
	SphereComponent->SetSimulatePhysics(false);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMeshComponent->SetupAttachment(SphereComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	FloatingPawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>("FloatingPawnMovement");
	FloatingPawnMovementComponent->MaxSpeed = 1000.f;
	FloatingPawnMovementComponent->Acceleration = 4000.f;
	FloatingPawnMovementComponent->Deceleration = 4000.f;
	FloatingPawnMovementComponent->TurningBoost = 8.f;
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AttributeSet = CreateDefaultSubobject<UAttributeSet>("AttributeSet");
	
	bReplicates = true;
	SetReplicateMovement(true);
}

void ANSBaseCompanionAI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		MaintainAltitude(DeltaSeconds);
		DroneAIRotate(DeltaSeconds);
	}
}

void ANSBaseCompanionAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ANSBaseCompanionAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!HasAuthority() || !IsValid(NewController)) return;
	
	ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>(NewController);
	if (!IsValid(DroneAIController)) return;
	
	CachedAIController = DroneAIController;
}

void ANSBaseCompanionAI::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ANSBaseCompanionAI::MoveTowards(const FVector& TargetLocation)
{
	FVector TargetPosition = TargetLocation - GetActorLocation();
	TargetPosition.Z = 0.f;
	
	if (TargetPosition.SizeSquared() < FMath::Square(ArrivalRadius)) return;
	
	FVector TargetDirection = TargetPosition.GetSafeNormal();
	TargetDirection += ComputeAvoidanceVector() * AvoidanceStrength;
	TargetDirection = TargetDirection.GetSafeNormal2D();
	
	AddMovementInput(TargetDirection, 1.0f);
}

void ANSBaseCompanionAI::SetOwnerPlayer(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	
	OwnerPlayer = Actor;
}

void ANSBaseCompanionAI::MaintainAltitude(float DeltaSeconds)
{
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f,0.f,GroundTraceDistance);
	
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Visibility, CollisionParams))
	{
		const float DesiredHeight = Hit.ImpactPoint.Z + Altitude;
		const float DesiredMoveDistance = DesiredHeight - Start.Z;
		
		if (FMath::Abs(DesiredMoveDistance) > AltitudeDeadZone)
		{
			const float InputZ = FMath::Clamp(DesiredMoveDistance / AltitudeCorrectionRange, -1.f, 1.f);
			AddMovementInput(FVector::UpVector, InputZ);
		}
	}
}

void ANSBaseCompanionAI::DroneAIRotate(float DeltaSeconds)
{
	if (!FloatingPawnMovementComponent) return;
	
	FVector Vel = FloatingPawnMovementComponent->Velocity;
	Vel.Z = 0.f;
	if (Vel.SizeSquared() < FMath::Square(MinSpeedToRotate)) return;
	
	const FRotator Current = GetActorRotation();
	const FRotator Desired(0.f, Vel.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpTo(Current, Desired,DeltaSeconds,YawInterpSpeed);
	SetActorRotation(NewRot);
}

#pragma region 회피기능

void ANSBaseCompanionAI::InitSteeringDirections()
{
	float AngleStep = 360.0f / NumSteeringDirections;
	
	for (int32 i = 0; i < NumSteeringDirections; ++i)
	{
		float Angle = i * AngleStep;
		
		FRotator Rotation = FRotator(0.f,Angle,0.f);
		FVector Direction = Rotation.Vector();
		
		SteeringDirections.Add(Direction);
	}
	
	
	
}

void ANSBaseCompanionAI::BuildInterestMap(const FVector& DesiredDirection)
{
	
	
}

void ANSBaseCompanionAI::BuildDangerMap()
{
	Dangermap.Reserve(SteeringDirections.Num());
}

FVector ANSBaseCompanionAI::ChooseSteeringDirection() const
{
	return FVector::ZeroVector;
}

#pragma endregion