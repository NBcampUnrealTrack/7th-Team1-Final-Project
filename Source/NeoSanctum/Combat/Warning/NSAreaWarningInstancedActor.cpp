// Copyright 2026 One Team. All rights reserved.

#include "NSAreaWarningInstancedActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ANSAreaWarningInstancedActor::ANSAreaWarningInstancedActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;

	InstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedWarningMesh"));
	SetRootComponent(InstancedMeshComponent);

	InstancedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InstancedMeshComponent->SetCastShadow(false);
	InstancedMeshComponent->bReceivesDecals = false;
	InstancedMeshComponent->SetTranslucentSortPriority(20);
	InstancedMeshComponent->SetCanEverAffectNavigation(false);
	InstancedMeshComponent->NumCustomDataFloats = InstanceCustomDataFloatCount;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));

	if (PlaneMeshAsset.Succeeded())
	{
		InstancedMeshComponent->SetStaticMesh(PlaneMeshAsset.Object);
	}
}

void ANSAreaWarningInstancedActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	UpdatePresentation();

	if (ElapsedTime >= Duration)
	{
		SetActorTickEnabled(false);
		Destroy();
	}
}

void ANSAreaWarningInstancedActor::InitializeCircleWarnings(
	const TArray<FVector>& InWorldLocations,
	const float InRadius,
	const float InDuration)
{
	const float Diameter = FMath::Max(InRadius * 2.0f, 1.0f);
	ShapeType = ENSAreaWarningPlaneShape::Circle;
	InitializeWarningBatch(InWorldLocations, FVector2D(Diameter, Diameter), InDuration);
}

void ANSAreaWarningInstancedActor::InitializeBoxWarnings(
	const TArray<FVector>& InWorldLocations,
	const FVector2D& InWorldSize,
	const float InDuration)
{
	ShapeType = ENSAreaWarningPlaneShape::Box;
	InitializeWarningBatch(InWorldLocations, InWorldSize, InDuration);
}

void ANSAreaWarningInstancedActor::InitializeWarningBatch(
	const TArray<FVector>& InWorldLocations,
	const FVector2D& InWorldSize,
	const float InDuration)
{
	ClearWarnings();

	Duration = FMath::Max(InDuration, 0.01f);
	ElapsedTime = 0.0f;

	const FVector2D WorldSize(
		FMath::Max(InWorldSize.X, 1.0f),
		FMath::Max(InWorldSize.Y, 1.0f));

	if (InWorldLocations.IsEmpty())
	{
		SetLifeSpan(0.01f);
		return;
	}

	InstancedMeshComponent->NumCustomDataFloats = FMath::Max(InstanceCustomDataFloatCount, 2);

	CreateMaterialInstance();

	const float SpawnWorldTime = GetCurrentWorldTimeSeconds();

	for (const FVector& WorldLocation : InWorldLocations)
	{
		AddWarningInstance(WorldLocation, WorldSize, SpawnWorldTime, Duration);
	}

	InstancedMeshComponent->MarkRenderStateDirty();

	UpdatePresentation();
	SetActorTickEnabled(true);
	SetLifeSpan(Duration + 0.1f);
}

void ANSAreaWarningInstancedActor::ClearWarnings()
{
	SetActorTickEnabled(false);

	if (InstancedMeshComponent)
	{
		InstancedMeshComponent->ClearInstances();
	}

	ElapsedTime = 0.0f;
	SetLifeSpan(0.0f);
}

void ANSAreaWarningInstancedActor::CreateMaterialInstance()
{
	UMaterialInterface* BaseMaterial = WarningMaterial.Get();

	if (!BaseMaterial && InstancedMeshComponent)
	{
		BaseMaterial = InstancedMeshComponent->GetMaterial(0);
	}

	if (!BaseMaterial)
	{
		return;
	}

	WarningMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);

	if (WarningMID && InstancedMeshComponent)
	{
		InstancedMeshComponent->SetMaterial(0, WarningMID);
	}
}

void ANSAreaWarningInstancedActor::UpdatePresentation()
{
	const float Progress = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

	float BlinkAlpha = 1.0f;
	if (bBlink)
	{
		const float Wave =
			0.5f + 0.5f * FMath::Sin(ElapsedTime * BlinkFrequency * UE_TWO_PI);

		BlinkAlpha = FMath::Lerp(BlinkMinAlpha, BlinkMaxAlpha, Wave);
	}

	ApplyMaterialParameters(Progress, BlinkAlpha);
}

void ANSAreaWarningInstancedActor::ApplyMaterialParameters(
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
	WarningMID->SetScalarParameterValue(TEXT("OpacityValue"), Opacity);
	WarningMID->SetScalarParameterValue(TEXT("Brightness"), Brightness);
	WarningMID->SetScalarParameterValue(TEXT("EdgeSoftness"), EdgeSoftness);
	WarningMID->SetScalarParameterValue(TEXT("RingThickness"), RingThickness);

	WarningMID->SetVectorParameterValue(TEXT("StartColor"), StartColor);
	WarningMID->SetVectorParameterValue(TEXT("EndColor"), EndColor);
}

void ANSAreaWarningInstancedActor::AddWarningInstance(
	const FVector& WorldLocation,
	const FVector2D& WorldSize,
	const float SpawnWorldTime,
	const float WarningDuration)
{
	if (!InstancedMeshComponent || WorldLocation.ContainsNaN())
	{
		return;
	}

	const float SafePlaneMeshSize = FMath::Max(PlaneMeshSize, 1.0f);
	const FVector RelativeLocation = WorldLocation - GetActorLocation();

	const FVector InstanceScale(
		WorldSize.X / SafePlaneMeshSize,
		WorldSize.Y / SafePlaneMeshSize,
		1.0f);

	const FTransform InstanceTransform(
		FRotator::ZeroRotator,
		RelativeLocation,
		InstanceScale);

	const int32 InstanceIndex = InstancedMeshComponent->AddInstance(InstanceTransform);

	if (InstanceIndex == INDEX_NONE)
	{
		return;
	}

	InstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, SpawnWorldTime, false);
	InstancedMeshComponent->SetCustomDataValue(InstanceIndex, 1, WarningDuration, false);
}

float ANSAreaWarningInstancedActor::GetCurrentWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();

	return World ? World->GetTimeSeconds() : 0.0f;
}
