// Copyright 2026 One Team. All rights reserved.


#include "NSDroneAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

ANSDroneAI::ANSDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;
	
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;
	GetCharacterMovement()->AvoidanceWeight = 0.2f;
	
	GetMesh()->SetEnableGravity(true);
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



