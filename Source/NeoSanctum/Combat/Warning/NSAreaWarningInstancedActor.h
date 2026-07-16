// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Combat/Warning/NSAreaWarningPlaneActor.h"
#include "NSAreaWarningInstancedActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/*
 * 작성자 : 최준혁
 *
 * 파일 생성일 : 26.07.15
 *
 * 클래스 개요 : 대량 범위 경고를 하나의 Instanced Static Mesh 컴포넌트로 표시하는 액터
 * 포격 패턴처럼 여러 위치에 동시에 표시되는 Warning Plane의 Actor, Tick, MID 생성 비용을 줄이는 역할
*/
UCLASS(Blueprintable)
class NEOSANCTUM_API ANSAreaWarningInstancedActor : public AActor
{
	GENERATED_BODY()

public:
	ANSAreaWarningInstancedActor();

	// Instanced 경고 표시 진행도를 갱신하는 함수
	virtual void Tick(float DeltaSeconds) override;

	// 여러 원형 경고를 Instanced Mesh로 초기화하는 함수
	void InitializeCircleWarnings(
		const TArray<FVector>& InWorldLocations,
		float InRadius,
		float InDuration);

	// 여러 박스 경고를 Instanced Mesh로 초기화하는 함수
	void InitializeBoxWarnings(
		const TArray<FVector>& InWorldLocations,
		const FVector2D& InWorldSize,
		float InDuration);

	// 여러 경고 위치와 크기를 Instanced Mesh로 초기화하는 함수
	void InitializeWarningBatch(
		const TArray<FVector>& InWorldLocations,
		const FVector2D& InWorldSize,
		float InDuration);

	// 생성된 경고 Instance를 모두 제거하는 함수
	void ClearWarnings();

private:
	// 경고 표시용 Instance Mesh 컴포넌트를 저장하는 변수
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> InstancedMeshComponent;

protected:
	// Instanced 경고에 사용할 머티리얼을 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Warning|Material")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	// Instanced 경고의 Shape 방식을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape")
	ENSAreaWarningPlaneShape ShapeType = ENSAreaWarningPlaneShape::Circle;

	// Instanced 경고의 Fill 방식을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape")
	ENSAreaWarningPlaneFillMode FillMode = ENSAreaWarningPlaneFillMode::Filled;

	// Instanced 경고의 가장자리 부드러움 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape", meta = (ClampMin = "0.0"))
	float EdgeSoftness = 0.025f;

	// Instanced 경고의 링 두께 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Shape", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RingThickness = 0.08f;

	// Instanced 경고의 시작 색상을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual")
	FLinearColor StartColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);

	// Instanced 경고의 종료 색상을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual")
	FLinearColor EndColor = FLinearColor(1.0f, 0.02f, 0.0f, 1.0f);

	// Instanced 경고의 투명도 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual", meta = (ClampMin = "0.0"))
	float Opacity = 0.45f;

	// Instanced 경고의 밝기 값을 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Visual", meta = (ClampMin = "0.0"))
	float Brightness = 1.5f;

	// Instanced 경고의 깜빡임 사용 여부를 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink")
	bool bBlink = false;

	// Instanced 경고의 깜빡임 주파수를 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.01"))
	float BlinkFrequency = 6.0f;

	// Instanced 경고의 최소 깜빡임 Alpha를 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.0", ClampMax = "1.0"))
	float BlinkMinAlpha = 0.25f;

	// Instanced 경고의 최대 깜빡임 Alpha를 저장하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warning|Blink",
		meta = (EditCondition = "bBlink", ClampMin = "0.0", ClampMax = "1.0"))
	float BlinkMaxAlpha = 1.0f;

	// 기본 Plane Mesh 한 변의 월드 크기를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Warning|Mesh", meta = (ClampMin = "1.0"))
	float PlaneMeshSize = 100.0f;

	// Instance Custom Data Float 개수를 저장하는 변수
	UPROPERTY(EditDefaultsOnly, Category = "Warning|Mesh", meta = (ClampMin = "2"))
	int32 InstanceCustomDataFloatCount = 2;

private:
	// Instanced 경고가 공유할 Dynamic Material Instance를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WarningMID;

	// Instanced 경고 표시 시간을 저장하는 변수
	float Duration = 1.0f;

	// Instanced 경고가 표시된 누적 시간을 저장하는 변수
	float ElapsedTime = 0.0f;

	// Instanced 경고 머티리얼 인스턴스를 생성하는 함수
	void CreateMaterialInstance();

	// Instanced 경고 표시 상태를 갱신하는 함수
	void UpdatePresentation();

	// Instanced 경고 머티리얼 공통 파라미터를 적용하는 함수
	void ApplyMaterialParameters(float Progress, float BlinkAlpha);

	// 단일 경고 Instance를 추가하는 함수
	void AddWarningInstance(
		const FVector& WorldLocation,
		const FVector2D& WorldSize,
		float SpawnWorldTime,
		float WarningDuration);

	// 현재 월드 시간을 반환하는 함수
	float GetCurrentWorldTimeSeconds() const;
};
