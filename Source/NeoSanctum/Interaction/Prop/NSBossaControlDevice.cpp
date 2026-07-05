// Copyright 2026 One Team. All rights reserved.


#include "NSBossaControlDevice.h"


// Sets default values
ANSBossaControlDevice::ANSBossaControlDevice()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANSBossaControlDevice::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANSBossaControlDevice::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

