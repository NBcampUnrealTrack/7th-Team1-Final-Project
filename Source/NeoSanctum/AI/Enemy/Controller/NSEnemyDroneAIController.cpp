// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDroneAIController.h"


// Sets default values
ANSEnemyDroneAIController::ANSEnemyDroneAIController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSEnemyDroneAIController::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSEnemyDroneAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

