// Copyright 2026 One Team. All rights reserved.


#include "NSCompanionDroneAI.h"


// Sets default values
ANSCompanionDroneAI::ANSCompanionDroneAI()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSCompanionDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSCompanionDroneAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANSCompanionDroneAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

