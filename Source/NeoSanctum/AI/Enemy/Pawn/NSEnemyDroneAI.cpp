// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDroneAI.h"


// Sets default values
ANSEnemyDroneAI::ANSEnemyDroneAI()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSEnemyDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSEnemyDroneAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANSEnemyDroneAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

