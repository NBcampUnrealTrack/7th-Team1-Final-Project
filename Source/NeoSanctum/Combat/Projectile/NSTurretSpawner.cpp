// Copyright 2026 One Team. All rights reserved.

#include "NSTurretSpawner.h"

#include "GameFramework/ProjectileMovementComponent.h"

ANSTurretSpawner::ANSTurretSpawner()
{
	UProjectileMovementComponent* Movement = GetProjectileMovementComponent();
	if (Movement)
	{
		Movement->bShouldBounce = true;
		// 탄력
		Movement->Bounciness = 0.55f;
		// 마찰력
		Movement->Friction = 0.2f;
	}
}

void ANSTurretSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	UProjectileMovementComponent* Movement = GetProjectileMovementComponent();
	if (Movement)
	{
		Movement->OnProjectileBounce.AddDynamic(this, &ThisClass::OnProjectileBounce)
	}
}

void ANSTurretSpawner::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// TODO : Projectile이 바운스 되는 순간 경사면의 Z축 경사 정도를 파악해서 Turret Spawn
}
