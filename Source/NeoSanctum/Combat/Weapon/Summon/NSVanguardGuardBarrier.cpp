// Copyright 2026 One Team. All rights reserved.

#include "NSVanguardGuardBarrier.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ANSVanguardGuardBarrier::ANSVanguardGuardBarrier()
{
	BoxBarrierCollisionComponent =
		CreateDefaultSubobject<UBoxComponent>(TEXT("BarrierCollisionComponent"));
	BoxBarrierCollisionComponent->InitBoxExtent(FVector(
		DefaultRadius * BoxDepthRatio,
		DefaultRadius * BoxWidthRatio,
		DefaultRadius * BoxHeightRatio
	));
	InitializeBarrierCollisionComponent(BoxBarrierCollisionComponent);
}

void ANSVanguardGuardBarrier::ApplyCollisionRadius(float Radius)
{
	if (BoxBarrierCollisionComponent)
	{
		BoxBarrierCollisionComponent->SetBoxExtent(FVector(
			Radius * BoxDepthRatio,
			Radius * BoxWidthRatio,
			Radius * BoxHeightRatio
		));
	}
}

void ANSVanguardGuardBarrier::ApplyVisualRadius(float Radius)
{
	if (!BarrierFlashMeshComponent)
	{
		return;
	}

	FVector BaseVisualExtent(DefaultRadius);
	if (const UStaticMesh* FlashMesh = BarrierFlashMeshComponent->GetStaticMesh())
	{
		BaseVisualExtent = FlashMesh->GetBounds().BoxExtent;
	}

	const FVector TargetVisualExtent(
		Radius * BoxDepthRatio,
		Radius * BoxWidthRatio,
		Radius * BoxHeightRatio
	);

	const FVector VisualScale(
		TargetVisualExtent.X / FMath::Max(BaseVisualExtent.X, KINDA_SMALL_NUMBER),
		TargetVisualExtent.Y / FMath::Max(BaseVisualExtent.Y, KINDA_SMALL_NUMBER),
		TargetVisualExtent.Z / FMath::Max(BaseVisualExtent.Z, KINDA_SMALL_NUMBER)
	);

	BarrierFlashMeshComponent->SetRelativeScale3D(VisualScale);
}
