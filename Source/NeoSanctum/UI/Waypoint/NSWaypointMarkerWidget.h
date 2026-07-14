// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSWaypointMarkerWidget.generated.h"

class UImage;
class UCommonTextBlock;
class UNSWaypointMarkerComponent;
struct FStreamableHandle;

/**
 * 웨이포인트 마커 1개의 비주얼 (아이콘 + 거리 텍스트)
 * 위치/거리 계산은 컨테이너 위젯이 담당하고, 이 위젯은 표시만 한다
 * 화면 밖 대상은 컨테이너가 가장자리 클램프로 방향을 표현한다 (별도 화살표 없음)
 */
UCLASS()
class NEOSANCTUM_API UNSWaypointMarkerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 마커 컴포넌트 연결 + 아이콘 비동기 로드 시작
	void InitializeMarker(UNSWaypointMarkerComponent* InMarker);

	// 거리 텍스트 갱신. 정수 미터가 바뀔 때만 SetText (Slate 무효화 최소화)
	void UpdateDistance(float DistanceMeters);

protected:
	virtual void NativeDestruct() override;

private:
	// 마커 아이콘 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	// 거리 텍스트 ("10M")
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DistanceText;

	// 연결된 마커 컴포넌트 (액터 파괴 대비 약참조)
	TWeakObjectPtr<UNSWaypointMarkerComponent> MarkerComponent;

	// 마지막으로 표시한 정수 미터 (불필요한 SetText 방지용 캐시)
	int32 LastDisplayedMeters = -1;

	// 아이콘 비동기 로드 핸들 (위젯 소멸 시 로드 취소용)
	TSharedPtr<FStreamableHandle> IconLoadHandle;
};
