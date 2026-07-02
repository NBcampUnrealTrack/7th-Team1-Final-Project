// Copyright 2026 One Team. All rights reserved.


#include "NSFlyingEnemyBase.h"


// Sets default values
ANSFlyingEnemyBase::ANSFlyingEnemyBase()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSFlyingEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSFlyingEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANSFlyingEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

