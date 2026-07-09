// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSAreaWarningPlaneActor.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ENSAreaWarningPlaneShape : uint8
{
	Circle,
	Box
};

UENUM(BlueprintType)
enum class ENSAreaWarningPlaneFillMode : uint8
{
	Filled,
	Ring
};

UCLASS()
class NEOSANCTUM_API ANSAreaWarningPlaneActor : public AActor
{
	GENERATED_BODY()

public:
	ANSAreaWarningPlaneActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeWarning(
		const FVector2D& InWorldSize,
		float InDuration);

	void InitializeCircleWarning(
		float InRadius,
		float InDuration);

	void InitializeBoxWarning(
		const FVector2D& InWorldSize,
		float InDuration);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PlaneMeshComponent;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Warning|Material")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape")
	ENSAreaWarningPlaneShape ShapeType = ENSAreaWarningPlaneShape::Circle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape")
	ENSAreaWarningPlaneFillMode FillMode = ENSAreaWarningPlaneFillMode::Filled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape", meta = (ClampMin = "0.0"))
	float EdgeSoftness = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RingThickness = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual")
	FLinearColor StartColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual")
	FLinearColor EndColor = FLinearColor(1.0f, 0.02f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual", meta = (ClampMin = "0.0"))
	float Opacity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual", meta = (ClampMin = "0.0"))
	float Brightness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink")
	bool bBlink = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.01"))
	float BlinkFrequency = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.0", ClampMax = "1.0"))
	float BlinkMinAlpha = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.0", ClampMax = "1.0"))
	float BlinkMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Animation")
	bool bAnimateSize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Animation",
		meta = (EditCondition = "bAnimateSize", ClampMin = "0.0"))
	float InitialSizeRatio = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Animation",
		meta = (EditCondition = "bAnimateSize", ClampMin = "0.01", ClampMax = "1.0"))
	float GrowTimeRatio = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Warning|Mesh", meta = (ClampMin = "1.0"))
	float PlaneMeshSize = 100.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WarningMID;

	FVector2D TargetWorldSize = FVector2D(100.0f, 100.0f);
	float Duration = 1.0f;
	float ElapsedTime = 0.0f;

	void CreateMaterialInstance();
	void UpdatePresentation();
	void ApplyMaterialParameters(float Progress, float BlinkAlpha);
};
