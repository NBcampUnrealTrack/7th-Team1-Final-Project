// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NSWaypointSubsystem.generated.h"

class UNSWaypointMarkerComponent;

// 마커 목록이 바뀔 때(등록/해제) UI에 알리는 델리게이트
DECLARE_MULTICAST_DELEGATE(FNSOnWaypointListChanged);

/**
 * 클라 로컬 웨이포인트 마커 레지스트리
 * 마커 컴포넌트가 스스로 등록/해제하고, HUD 컨테이너 위젯이 목록을 소비
 */
UCLASS()
class NEOSANCTUM_API UNSWaypointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 마커 등록 (중복 등록은 무시)
	void RegisterMarker(UNSWaypointMarkerComponent* Marker);

	// 마커 해제 (미등록 마커는 무시)
	void UnregisterMarker(UNSWaypointMarkerComponent* Marker);

	// 현재 등록된 마커 목록 (스테일 포인터는 소비 측 틱에서 걸러냄)
	const TArray<TWeakObjectPtr<UNSWaypointMarkerComponent>>& GetMarkers() const { return Markers; }

	// 목록 변경 알림 (컨테이너 위젯이 구독)
	FNSOnWaypointListChanged OnWaypointListChanged;

private:
	// 액터 파괴 시 자동으로 무효화되도록 약참조로 보관
	TArray<TWeakObjectPtr<UNSWaypointMarkerComponent>> Markers;
};
