// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDrone.h"


// Sets default values
ANSEnemyDrone::ANSEnemyDrone()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSEnemyDrone::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSEnemyDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANSEnemyDrone::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

