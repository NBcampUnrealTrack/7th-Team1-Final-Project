// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/UI/Waypoint/NSWaypointContainerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "NeoSanctum/UI/Waypoint/NSWaypointMarkerComponent.h"
#include "NeoSanctum/UI/Waypoint/NSWaypointMarkerWidget.h"
#include "NeoSanctum/UI/Waypoint/NSWaypointSubsystem.h"

void UNSWaypointContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 마커 목록 변경을 구독하고, 이미 등록돼 있던 마커도 즉시 반영
	if (UNSWaypointSubsystem* Subsystem = GetWaypointSubsystem())
	{
		ListChangedHandle = Subsystem->OnWaypointListChanged.AddUObject(
			this, &UNSWaypointContainerWidget::RefreshMarkerWidgets);
	}

	RefreshMarkerWidgets();
}

void UNSWaypointContainerWidget::NativeDestruct()
{
	// 위젯 소멸 후 델리게이트 호출 방지
	if (UNSWaypointSubsystem* Subsystem = GetWaypointSubsystem())
	{
		Subsystem->OnWaypointListChanged.Remove(ListChangedHandle);
	}

	Super::NativeDestruct();
}

void UNSWaypointContainerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	// 뷰포트 크기(픽셀)와 DPI 스케일 — 투영 결과를 UMG 좌표로 변환할 때 필요
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	if (ViewportSize.IsNearlyZero() || ViewportScale <= 0.f)
	{
		return;
	}

	// 카메라 시점 — "카메라 뒤" 판정에 사용
	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector();

	// 거리는 플레이어 폰 기준 (폰이 없으면 카메라 기준으로 폴백)
	const APawn* LocalPawn = PlayerController->GetPawn();
	const FVector DistanceOrigin =
		LocalPawn ? LocalPawn->GetActorLocation() : CameraLocation;

	const FVector2D ScreenCenter = ViewportSize * 0.5f;
	const float PaddingPx = EdgePadding * ViewportScale;

	for (auto It = MarkerWidgets.CreateIterator(); It; ++It)
	{
		UNSWaypointMarkerComponent* Marker = It.Key().Get();
		UNSWaypointMarkerWidget* Widget = It.Value().Get();

		// 대상 액터가 파괴됐거나 위젯이 사라졌으면 정리
		if (!Marker || !Widget)
		{
			if (Widget)
			{
				Widget->RemoveFromParent();
			}
			It.RemoveCurrent();
			continue;
		}

		const FVector WorldLocation = Marker->GetMarkerWorldLocation();

		// --- 거리 갱신 (cm → m) ---
		const float DistanceMeters =
			FVector::Dist(DistanceOrigin, WorldLocation) / 100.f;
		Widget->UpdateDistance(DistanceMeters);

		// --- 거리 기반 축소: 멀수록 마커를 줄여 원근감과 어울리게 (Near 이하 1.0 → Far 이상 MinMarkerScale) ---
		const float MarkerScale = FMath::GetMappedRangeValueClamped(
			FVector2D(ScaleNearDistance, ScaleFarDistance),
			FVector2D(1.f, MinMarkerScale),
			DistanceMeters);
		Widget->SetRenderScale(FVector2D(MarkerScale, MarkerScale));

		// --- 화면 투영 ---
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		const bool bProjected = PlayerController->ProjectWorldLocationToScreen(
			WorldLocation, ScreenPosition, /*bPlayerViewportRelative=*/false);

		// 카메라 뒤 판정 — ProjectWorldLocationToScreen은 카메라 뒤에서
		// false를 반환하며 좌표를 갱신하지 않으므로 투영 결과를 신뢰할 수 없다
		const bool bBehindCamera =
			FVector::DotProduct(WorldLocation - CameraLocation, CameraForward) < 0.f;

		// 화면 안 판정: 투영 성공 + 카메라 앞 + 패딩 안쪽
		const bool bOnScreen = bProjected && !bBehindCamera
			&& ScreenPosition.X >= PaddingPx
			&& ScreenPosition.X <= ViewportSize.X - PaddingPx
			&& ScreenPosition.Y >= PaddingPx
			&& ScreenPosition.Y <= ViewportSize.Y - PaddingPx;

		// --- 화면 밖이면 가장자리 클램프 (마커 위치 자체가 방향을 표현) ---
		FVector2D FinalPosition = ScreenPosition;
		if (!bOnScreen)
		{
			FVector2D Direction;
			if (bProjected && !bBehindCamera)
			{
				// 카메라 앞이지만 화면 가장자리 밖: 투영 좌표 방향을 그대로 사용
				Direction = ScreenPosition - ScreenCenter;
			}
			else
			{
				// 카메라 뒤(투영 좌표 무효): 대상 방향을 카메라 공간으로 변환해
				// 화면 방향을 직접 계산 (카메라 기준 우 = +Y, 상 = +Z → 스크린 Y는 아래가 +라 부호 반전)
				const FVector LocalDir =
					CameraRotation.UnrotateVector(WorldLocation - CameraLocation);
				Direction = FVector2D(LocalDir.Y, -LocalDir.Z);
			}

			if (Direction.IsNearlyZero())
			{
				// 대상이 정확히 정후방에 있는 특이 케이스: 아래쪽 가장자리에 표시
				Direction = FVector2D(0.f, 1.f);
			}
			Direction.Normalize();

			const FVector2D HalfExtent(
				ViewportSize.X * 0.5f - PaddingPx,
				ViewportSize.Y * 0.5f - PaddingPx);
			const float ScaleX = !FMath::IsNearlyZero(Direction.X)
				? HalfExtent.X / FMath::Abs(Direction.X)
				: TNumericLimits<float>::Max();
			const float ScaleY = !FMath::IsNearlyZero(Direction.Y)
				? HalfExtent.Y / FMath::Abs(Direction.Y)
				: TNumericLimits<float>::Max();

			FinalPosition = ScreenCenter + Direction * FMath::Min(ScaleX, ScaleY);
		}

		// --- 배치: 픽셀 좌표를 DPI 스케일로 나눠 UMG 좌표로 변환 ---
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			CanvasSlot->SetPosition(FinalPosition / ViewportScale);
		}
	}
}

void UNSWaypointContainerWidget::RefreshMarkerWidgets()
{
	UNSWaypointSubsystem* Subsystem = GetWaypointSubsystem();
	if (!Subsystem || !MarkerWidgetClass || !MarkerCanvas)
	{
		return;
	}

	const TArray<TWeakObjectPtr<UNSWaypointMarkerComponent>>& RegisteredMarkers =
		Subsystem->GetMarkers();

	// 1) 새로 등록된 마커의 위젯 생성
	for (const TWeakObjectPtr<UNSWaypointMarkerComponent>& WeakMarker : RegisteredMarkers)
	{
		UNSWaypointMarkerComponent* Marker = WeakMarker.Get();
		if (!Marker || MarkerWidgets.Contains(WeakMarker))
		{
			continue;
		}

		UNSWaypointMarkerWidget* NewWidget =
			CreateWidget<UNSWaypointMarkerWidget>(this, MarkerWidgetClass);
		if (!NewWidget)
		{
			continue;
		}

		NewWidget->InitializeMarker(Marker);

		// 중심 정렬(0.5, 0.5)로 아이콘이 투영 좌표 정중앙에 오도록 배치
		if (UCanvasPanelSlot* CanvasSlot = MarkerCanvas->AddChildToCanvas(NewWidget))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}

		MarkerWidgets.Add(WeakMarker, NewWidget);
	}

	// 2) 레지스트리에서 빠진(비활성/파괴된) 마커의 위젯 제거
	for (auto It = MarkerWidgets.CreateIterator(); It; ++It)
	{
		const bool bStillRegistered =
			It.Key().IsValid() && RegisteredMarkers.Contains(It.Key());
		if (bStillRegistered)
		{
			continue;
		}

		if (UNSWaypointMarkerWidget* Widget = It.Value().Get())
		{
			Widget->RemoveFromParent();
		}
		It.RemoveCurrent();
	}
}

UNSWaypointSubsystem* UNSWaypointContainerWidget::GetWaypointSubsystem() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UNSWaypointSubsystem>();
}
