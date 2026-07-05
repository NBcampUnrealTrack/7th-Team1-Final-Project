// Copyright 2026 One Team. All rights reserved.


#include "NSBossControlDevice.h"


// Sets default values
ANSBossControlDevice::ANSBossControlDevice()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSBossControlDevice::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSBossControlDevice::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

