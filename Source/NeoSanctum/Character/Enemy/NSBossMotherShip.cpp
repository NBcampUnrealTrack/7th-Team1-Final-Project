// Copyright 2026 One Team. All rights reserved.


#include "NSBossMotherShip.h"


// Sets default values
ANSBossMotherShip::ANSBossMotherShip()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSBossMotherShip::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSBossMotherShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANSBossMotherShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

