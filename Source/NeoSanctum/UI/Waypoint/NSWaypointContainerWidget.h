// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSWaypointContainerWidget.generated.h"

class UCanvasPanel;
class UNSWaypointMarkerComponent;
class UNSWaypointMarkerWidget;
class UNSWaypointSubsystem;

/**
 * 등록된 웨이포인트 마커들을 관리하는 전체 화면 컨테이너
 * 서브시스템 델리게이트로 마커 위젯을 생성/제거하고,
 * 매 틱 스크린 투영·가장자리 클램프·거리 텍스트를 갱신
 */
UCLASS()
class NEOSANCTUM_API UNSWaypointContainerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// 서브시스템 목록과 마커 위젯 맵을 동기화 (생성/제거)
	void RefreshMarkerWidgets();

	UNSWaypointSubsystem* GetWaypointSubsystem() const;

	// 마커 1개 비주얼 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Waypoint", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UNSWaypointMarkerWidget> MarkerWidgetClass;

	// 화면 가장자리 클램프 여백
	UPROPERTY(EditAnywhere, Category = "Waypoint", meta = (AllowPrivateAccess = "true"))
	float EdgePadding = 48.f;

	// 이 거리(m) 이하면 마커 스케일 1.0
	UPROPERTY(EditAnywhere, Category = "Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float ScaleNearDistance = 10.f;

	// 이 거리(m) 이상이면 MinMarkerScale로 고정
	UPROPERTY(EditAnywhere, Category = "Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float ScaleFarDistance = 30.f;

	// 최대 거리에서의 마커 스케일
	UPROPERTY(EditAnywhere, Category = "Waypoint", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1.0"))
	float MinMarkerScale = 0.6f;

	// 마커 위젯을 배치할 전체 화면 캔버스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MarkerCanvas;

	// 마커 컴포넌트 → 마커 위젯 매핑.
	// 위젯 소유권은 MarkerCanvas 슬롯이 가지므로 여기는 약참조로 충분
	TMap<TWeakObjectPtr<UNSWaypointMarkerComponent>, TWeakObjectPtr<UNSWaypointMarkerWidget>> MarkerWidgets;

	// 서브시스템 델리게이트 구독 해제용 핸들
	FDelegateHandle ListChangedHandle;
};
