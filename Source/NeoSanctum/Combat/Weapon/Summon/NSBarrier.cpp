// Copyright 2026 One Team. All rights reserved.


#include "NSBarrier.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"

ANSBarrier::ANSBarrier()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	BarrierCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("BarrierCollisionComponent"));
	SetRootComponent(BarrierCollisionComponent);
	BarrierCollisionComponent->InitSphereRadius(DefaultRadius);
	BarrierCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrierCollisionComponent->SetGenerateOverlapEvents(false);

	BarrierNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BarrierNiagaraComponent"));
	BarrierNiagaraComponent->SetupAttachment(BarrierCollisionComponent);
	BarrierNiagaraComponent->SetAutoActivate(true);
}

void ANSBarrier::InitializeBarrier(
	APawn* InOwningPawn,
	AController* InOwningController,
	float InRadius)
{
	OwningPawn = InOwningPawn;
	OwningController = InOwningController;

	if (OwningPawn)
	{
		SetOwner(OwningPawn);
		SetInstigator(OwningPawn);
	}

	ApplyRadius(InRadius);
}

void ANSBarrier::BeginPlay()
{
	Super::BeginPlay();

	ApplyRadius(DefaultRadius);
}

void ANSBarrier::ApplyRadius(float InRadius)
{
	const float Radius = FMath::Max(InRadius, MinimumRadius);

	if (BarrierCollisionComponent)
	{
		BarrierCollisionComponent->SetSphereRadius(Radius);
	}

	if (BarrierNiagaraComponent)
	{
		const float VisualScale = Radius / DefaultRadius;
		BarrierNiagaraComponent->SetRelativeScale3D(FVector(VisualScale));
		BarrierNiagaraComponent->SetVariableFloat(TEXT("User.BarrierRadius"), Radius);
	}
}
