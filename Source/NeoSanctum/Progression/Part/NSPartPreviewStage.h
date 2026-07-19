// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "NSPartPreviewStage.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class USkeletalMesh;

/**
 * 파츠 Detail 패널의 실시간 3D 프리뷰용 무대.
 * 월드 밖 먼 위치에 스폰되어 자기 자신만 캡처하는 RenderTarget을 만들어 제공한다.
 */
UCLASS()
class NEOSANCTUM_API ANSPartPreviewStage : public AActor
{
	GENERATED_BODY()

public:
	ANSPartPreviewStage();

	// 프리뷰할 메시 교체 (null이면 프리뷰 중지)
	void SetPreviewMesh(USkeletalMesh* Mesh);

	// 사용자 드래그에 의한 수동 회전 (Yaw 각도 델타만큼 즉시 회전 + 재캡처)
	void AddManualYaw(float DeltaYawDegrees);

	// 휠에 의한 줌 (양수 = 줌인). 프레이밍 거리에 배율을 곱해 카메라를 앞뒤로 이동 + 재캡처
	void AddZoom(float WheelDelta);

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	// 모든 파츠 메시를 미리 로드하고 메시별 상주 컴포넌트를 붙인 웜업 스테이지를 스폰한다.
	// 스테이지가 살아있는 동안 핸들과 bForceMipStreaming 컴포넌트가 유지되어
	// 파츠샵에서 어떤 파츠를 선택해도 첫 캡처부터 풀 밉 텍스처가 나온다
	static void WarmupAllPartMeshes(UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;

	// 회전하지 않는 고정 원점. 카메라/조명/메시가 전부 이 밑에 형제로 붙는다
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneComponent> StageRoot;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UPointLightComponent> RimLight;

	// 메시 뒤에 깔리는 단색 배경판 — 월드 스카이박스 대신 통제된 배경을 캡처하게 한다
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UStaticMeshComponent> BackdropComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	FLinearColor BackdropColor = FLinearColor(0.85f, 0.85f, 0.87f);

	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	int32 RenderTargetSize = 512;

	// 메시 바운즈 반지름 대비 카메라 거리 배수
	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	float CameraDistanceMultiplier = 3.2f;

	// 휠 한 칸당 줌 배율 변화량
	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	float ZoomStep = 0.12f;

	// 줌 배율 범위 (1 = 기본 프레이밍 거리)
	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	float MinZoomFactor = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	float MaxZoomFactor = 2.f;

private:
	// 새 메시의 실제 형상(바운즈) 중심을 기준으로 카메라/배경판을 재배치
	void FrameCameraOnMesh();

	// 현재 줌 배율을 반영해 프레이밍된 시점으로 카메라 이동
	void UpdateCameraLocation();

	// 웜업: 로드 완료된 메시를 상주 컴포넌트로 붙여 밉을 계속 강제 유지
	void AddWarmupMeshComponent(USkeletalMesh* Mesh);

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	// 밉 로드 완료 후 재캡처용 (메시 교체가 연달아 일어나면 이전 타이머는 덮어씀)
	FTimerHandle RecaptureTimerHandle;

	// FrameCameraOnMesh에서 확정된 프레이밍 (줌은 이 값에 배율만 곱한다)
	FVector FramedOrigin = FVector::ZeroVector;
	float FramedDistance = 0.f;
	float ZoomFactor = 1.f;

	// 웜업 스테이지 전용: 메시별 상주 컴포넌트와 로드 핸들 (스테이지 수명 동안 유지)
	UPROPERTY()
	TArray<TObjectPtr<USkeletalMeshComponent>> WarmupMeshComponents;

	TArray<TSharedPtr<FStreamableHandle>> WarmupLoadHandles;
};
