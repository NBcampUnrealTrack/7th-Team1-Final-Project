// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSCompassBarWidget.generated.h"

class UCommonTextBlock;
class UCanvasPanel;
class UImage;
class UTexture2D;
class ANSInteractableNPCBase;
class UNSMinimapIconComponent;
class UNSPlayerProgressComponent;

// Compass 월드 마커 표시 설정
USTRUCT(BlueprintType)
struct FNSCompassMarkerStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker")
	TObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker", meta = (ClampMin = "1.0"))
	FVector2D Size = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker")
	FLinearColor Color = FLinearColor::White;

	// 방위 텍스트 높이 기준 마커 Y축 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker")
	float VerticalOffset = 0.0f;
};

// 미니맵 Icon Row별 Compass 월드 마커 설정
USTRUCT(BlueprintType)
struct FNSCompassMarkerConfig
{
	GENERATED_BODY()

	// 미니맵 아이콘 DataTable Row 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker")
	FName IconRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker")
	FNSCompassMarkerStyle Style;
};

/**
 * 카메라 방향을 기준으로 눈금과 방위 문자를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSCompassBarWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 목표 방향의 나침반 X 좌표 계산
	UFUNCTION(BlueprintPure, Category = "Compass")
	bool CalculateCompassPosition(
		float ViewYaw,
		float TargetYaw,
		float BarWidth,
		float& OutX) const;

protected:
	// 카메라 Yaw 기준 눈금 및 방위 문자 갱신
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

	virtual void NativeDestruct() override;

private:
	// 360도 눈금 스트립 반복 배치
	// 동일한 360도 텍스처를 연속 배치하는 반복 구간 갱신
	void UpdateTickStrips(
		float ViewYaw,
		const FVector2D& CompassSize) const;

	// 8방위 문자 위치 갱신
	void UpdateDirectionLabels(
		float ViewYaw,
		float BarWidth) const;

	// 단일 방위 문자 위치 갱신
	// 가시 각도 범위에 따른 표시 여부 및 화면 좌표 반영
	void UpdateDirectionLabel(
		UCommonTextBlock* DirectionText,
		float RelativeYaw,
		float ViewYaw,
		float BarWidth) const;

	// 단일 눈금 스트립 위치 갱신
	// 반복 구간 연결을 위한 Canvas 슬롯 위치 및 크기 반영
	void UpdateTickStrip(
		UImage* TickStrip,
		float PositionX,
		float StripWidth,
		float FallbackHeight) const;

	// 등록된 미니맵 아이콘의 월드 마커 위치 갱신
	void UpdateWorldMarkers(
		const FVector& ViewLocation,
		float ViewYaw,
		const FVector2D& CompassSize);

	// Icon Row 기준 월드 마커 표시 설정 조회
	const FNSCompassMarkerStyle* FindWorldMarkerStyle(FName IconRowName) const;

	// 마커 컴포넌트 전용 Image 생성 및 조회
	UImage* FindOrCreateWorldMarkerImage(UNSMinimapIconComponent* IconComponent);

	// 동적 월드 마커 Image 정리
	void ClearWorldMarkerImages();

	// 방위 텍스트 기준 마커 Y 좌표 조회
	float GetWorldMarkerBaseY(const FVector2D& CompassSize) const;

	// 로컬 플레이어 진행도 조회
	const UNSPlayerProgressComponent* GetLocalPlayerProgressComponent() const;

	// 구조 완료 기준 NPC 마커 표시 여부 판정
	bool ShouldDrawNPCMarker(const ANSInteractableNPCBase& NPC) const;

	// 나침반 좌우 한쪽에 표시할 각도 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "180.0"))
	float VisibleHalfAngle = 90.0f;

	// 월드 북쪽으로 사용할 Yaw 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass", meta = (AllowPrivateAccess = "true"))
	float NorthWorldYaw = 0.0f;

	// Icon Row별 월드 마커 표시 설정
	// 배열에 등록되지 않은 Icon Row는 Compass에 표시하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker", meta = (AllowPrivateAccess = "true", TitleProperty = "IconRowName"))
	TArray<FNSCompassMarkerConfig> WorldMarkerConfigs;

	// 눈금 및 방위 텍스트 위에 표시할 마커 ZOrder
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass|World Marker", meta = (AllowPrivateAccess = "true"))
	int32 WorldMarkerZOrder = 10;

	// 나침반 표시 영역 캔버스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CompassCanvas;

	// 360도 눈금 반복 스트립
	// 기준 360도 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripA;

	// 오른쪽 첫 번째 반복 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripB;

	// 오른쪽 두 번째 반복 스트립
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TickStripC;

private:
	// 8방위 문자
	// WBP에서 선택적으로 바인딩하는 방위별 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionN;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionNE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionSE;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionS;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionSW;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionW;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DirectionNW;

	// 미니맵 등록 컴포넌트별 동적 Compass 마커 Image
	TMap<TWeakObjectPtr<UNSMinimapIconComponent>, TWeakObjectPtr<UImage>> WorldMarkerImages;
};
