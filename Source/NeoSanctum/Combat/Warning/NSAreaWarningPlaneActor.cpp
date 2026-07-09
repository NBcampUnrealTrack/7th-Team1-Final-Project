// Copyright 2026 One Team. All rights reserved.

#include "NSAreaWarningPlaneActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ANSAreaWarningPlaneActor::ANSAreaWarningPlaneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	PlaneMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	SetRootComponent(PlaneMeshComponent);

	PlaneMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaneMeshComponent->SetCastShadow(false);
	PlaneMeshComponent->bReceivesDecals = false;
	PlaneMeshComponent->SetTranslucentSortPriority(20);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));

	if (PlaneMeshAsset.Succeeded())
	{
		PlaneMeshComponent->SetStaticMesh(PlaneMeshAsset.Object);
	}
}

void ANSAreaWarningPlaneActor::InitializeWarning(
	const FVector2D& InWorldSize,
	float InDuration)
{
	TargetWorldSize.X = FMath::Max(InWorldSize.X, 1.0f);
	TargetWorldSize.Y = FMath::Max(InWorldSize.Y, 1.0f);

	Duration = FMath::Max(InDuration, 0.01f);
	ElapsedTime = 0.0f;

	CreateMaterialInstance();
	UpdatePresentation();

	SetLifeSpan(Duration + 0.1f);
}

void ANSAreaWarningPlaneActor::InitializeCircleWarning(
	float InRadius,
	float InDuration)
{
	const float Diameter = FMath::Max(InRadius * 2.0f, 1.0f);
	ShapeType = ENSAreaWarningPlaneShape::Circle;
	InitializeWarning(FVector2D(Diameter, Diameter), InDuration);
}

void ANSAreaWarningPlaneActor::InitializeBoxWarning(
	const FVector2D& InWorldSize,
	float InDuration)
{
	ShapeType = ENSAreaWarningPlaneShape::Box;
	InitializeWarning(InWorldSize, InDuration);
}

void ANSAreaWarningPlaneActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	UpdatePresentation();

	if (ElapsedTime >= Duration)
	{
		Destroy();
	}
}

void ANSAreaWarningPlaneActor::CreateMaterialInstance()
{
	if (WarningMaterial)
	{
		WarningMID = PlaneMeshComponent->CreateDynamicMaterialInstance(0, WarningMaterial);
	}
	else
	{
		WarningMID = PlaneMeshComponent->CreateDynamicMaterialInstance(0);
	}
}

void ANSAreaWarningPlaneActor::UpdatePresentation()
{
	const float Progress = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

	float SizeRatio = 1.0f;
	if (bAnimateSize)
	{
		const float GrowDuration = FMath::Max(Duration * GrowTimeRatio, 0.01f);
		const float GrowAlpha = FMath::Clamp(ElapsedTime / GrowDuration, 0.0f, 1.0f);
		SizeRatio = FMath::Lerp(InitialSizeRatio, 1.0f, GrowAlpha);
	}

	const FVector2D CurrentSize = TargetWorldSize * SizeRatio;

	PlaneMeshComponent->SetRelativeScale3D(FVector(
		CurrentSize.X / PlaneMeshSize,
		CurrentSize.Y / PlaneMeshSize,
		1.0f));

	float BlinkAlpha = 1.0f;
	if (bBlink)
	{
		const float Wave =
			0.5f + 0.5f * FMath::Sin(ElapsedTime * BlinkFrequency * UE_TWO_PI);

		BlinkAlpha = FMath::Lerp(BlinkMinAlpha, BlinkMaxAlpha, Wave);
	}

	ApplyMaterialParameters(Progress, BlinkAlpha);
}

void ANSAreaWarningPlaneActor::ApplyMaterialParameters(
	float Progress,
	float BlinkAlpha)
{
	if (!WarningMID)
	{
		return;
	}

	WarningMID->SetScalarParameterValue(
		TEXT("ShapeType"),
		static_cast<float>(static_cast<uint8>(ShapeType)));

	WarningMID->SetScalarParameterValue(
		TEXT("FillMode"),
		static_cast<float>(static_cast<uint8>(FillMode)));

	WarningMID->SetScalarParameterValue(TEXT("Progress"), Progress);
	WarningMID->SetScalarParameterValue(TEXT("BlinkAlpha"), BlinkAlpha);
	WarningMID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	WarningMID->SetScalarParameterValue(TEXT("Brightness"), Brightness);
	WarningMID->SetScalarParameterValue(TEXT("EdgeSoftness"), EdgeSoftness);
	WarningMID->SetScalarParameterValue(TEXT("RingThickness"), RingThickness);

	WarningMID->SetVectorParameterValue(TEXT("StartColor"), StartColor);
	WarningMID->SetVectorParameterValue(TEXT("EndColor"), EndColor);
}
