// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ANSDroneAIController::StaticClass();
	
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;
	
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;
	GetCharacterMovement()->AvoidanceWeight = 0.2f;
	
	GetMesh()->SetEnableGravity(true);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_GameTraceChannel1);
}

void ANSDroneAI::GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const
{
	Super::GetActorEyesViewPoint(Location, Rotation);
	Rotation = GetActorRotation();
}

void ANSDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	DroneAIController = Cast<ANSDroneAIController>(GetController());
	
	if (DroneAIController)
	{
		DroneAIBBComponent = DroneAIController->GetBlackboardComponent();
	}
}


