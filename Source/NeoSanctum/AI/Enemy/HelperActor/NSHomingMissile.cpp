// Copyright 2026 One Team. All rights reserved.


#include "NSHomingMissile.h"


// Sets default values
ANSHomingMissile::ANSHomingMissile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSHomingMissile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSHomingMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

