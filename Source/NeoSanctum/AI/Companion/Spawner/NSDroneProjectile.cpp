// Copyright 2026 One Team. All rights reserved.


#include "NSDroneProjectile.h"


// Sets default values
ANSDroneProjectile::ANSDroneProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSDroneProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSDroneProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

