// Copyright 2026 One Team. All rights reserved.

#include "NSTurret.h"

#include "Components/SphereComponent.h"

ANSTurret::ANSTurret()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComponent"));
	BaseMeshComponent->SetupAttachment(SceneRoot);

	HeadPivotComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivotComponent"));
	HeadPivotComponent->SetupAttachment(BaseMeshComponent);

	HeadMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMeshComponent"));
	HeadMeshComponent->SetupAttachment(HeadPivotComponent);

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphereComponent"));
	DetectionSphereComponent->SetupAttachment(SceneRoot);
	DetectionSphereComponent->InitSphereRadius(DetectionRadius);
	DetectionSphereComponent->SetGenerateOverlapEvents(true);
}

void ANSTurret::BeginPlay()
{
	Super::BeginPlay();

	if (DetectionSphereComponent)
	{
		DetectionSphereComponent->SetSphereRadius(DetectionRadius);
	}
}
