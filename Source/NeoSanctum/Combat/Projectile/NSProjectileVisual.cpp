// Copyright 2026 One Team. All rights reserved.


#include "NSProjectileVisual.h"

ANSProjectileVisual::ANSProjectileVisual()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
	AActor::SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(SceneRoot);

	// 모든 충돌 Off
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCanEverAffectNavigation(false);
}

void ANSProjectileVisual::ActivateVisual(const FVector& Location, const FVector& Direction)
{
	SetVisualTransform(Location, Direction);
	SetActorHiddenInGame(false);
}

void ANSProjectileVisual::DeactivateVisual()
{
	SetActorHiddenInGame(true);
}

void ANSProjectileVisual::SetVisualTransform(const FVector& Location, const FVector& Direction)
{
	const FRotator Rotation = Direction.Rotation();

	SetActorLocationAndRotation(
		Location,
		Rotation,
		false,                         // 충돌 검사 X
		nullptr,                       // 충돌 결과를 받을 포인터
		ETeleportType::TeleportPhysics // 순간이동으로 처리해 물리 속도 유지
	); 
}
