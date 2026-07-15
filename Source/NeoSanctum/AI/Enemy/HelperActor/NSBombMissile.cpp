// Copyright 2026 One Team. All rights reserved.


#include "NSBombMissile.h"


// Sets default values
ANSBombMissile::ANSBombMissile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSBombMissile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSBombMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

