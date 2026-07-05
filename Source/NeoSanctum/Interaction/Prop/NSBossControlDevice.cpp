// Copyright 2026 One Team. All rights reserved.


#include "NSBossControlDevice.h"


// Sets default values
ANSBossControlDevice::ANSBossControlDevice()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSBossControlDevice::OnServerDestroyed(const FVector& Origin)
{
	Super::OnServerDestroyed(Origin);
	
	OnControlDeviceDestroyed.Broadcast(this);
}


