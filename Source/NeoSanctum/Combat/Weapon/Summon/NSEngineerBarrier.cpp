// Copyright 2026 One Team. All rights reserved.

#include "NSEngineerBarrier.h"

#include "Components/SphereComponent.h"

ANSEngineerBarrier::ANSEngineerBarrier()
{
	SphereBarrierCollisionComponent =
		CreateDefaultSubobject<USphereComponent>(TEXT("BarrierCollisionComponent"));
	SphereBarrierCollisionComponent->InitSphereRadius(DefaultRadius);
	InitializeBarrierCollisionComponent(SphereBarrierCollisionComponent);
}

void ANSEngineerBarrier::ApplyCollisionRadius(float Radius)
{
	if (SphereBarrierCollisionComponent)
	{
		SphereBarrierCollisionComponent->SetSphereRadius(Radius);
	}
}
