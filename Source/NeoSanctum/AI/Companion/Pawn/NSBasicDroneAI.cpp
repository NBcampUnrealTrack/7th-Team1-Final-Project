// Copyright 2026 One Team. All rights reserved.


#include "NSBasicDroneAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

ANSBasicDroneAI::ANSBasicDroneAI()
{
	PrimaryActorTick.bCanEverTick = false;
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ANSDroneAIController::StaticClass();
}

void ANSBasicDroneAI::GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const
{
	Super::GetActorEyesViewPoint(Location, Rotation);
	Rotation = GetActorRotation();
}

void ANSBasicDroneAI::SetOwnerPlayer(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	
	OwnerPlayer = Actor;
}

void ANSBasicDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	CachedAIController = Cast<ANSDroneAIController>(GetController());
	
	if (CachedAIController)
	{
		DroneAIBBComponent = CachedAIController->GetBlackboardComponent();
	}
}


