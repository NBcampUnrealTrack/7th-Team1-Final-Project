// Copyright 2026 One Team. All rights reserved.


#include "NSBombMissile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


ANSBombMissile::ANSBombMissile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCanEverAffectNavigation(false);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetGenerateOverlapEvents(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bAutoActivate = false;
}

void ANSBombMissile::InitDrop(const FVector& LandingLocation)
{
	TargetLandingLocation = LandingLocation;
	bIsDropping = true;

	ProjectileMovement->Velocity = FVector(0.f, 0.f, -DropSpeed);
	ProjectileMovement->Activate();
}

void ANSBombMissile::BeginPlay()
{
	Super::BeginPlay();

	ProjectileMovement->ProjectileGravityScale = DropGravityScale;
}

void ANSBombMissile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsDropping) return;

	if (GetActorLocation().Z <= TargetLandingLocation.Z)
	{
		bIsDropping = false;
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();

		FVector Location = GetActorLocation();
		Location.Z = TargetLandingLocation.Z;
		SetActorLocation(Location);
	}
}