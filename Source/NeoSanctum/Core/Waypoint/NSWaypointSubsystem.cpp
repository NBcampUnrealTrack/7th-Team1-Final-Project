// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSWaypointSubsystem.h"

#include "NeoSanctum/Core/Waypoint/NSWaypointMarkerComponent.h"

void UNSWaypointSubsystem::RegisterMarker(UNSWaypointMarkerComponent* Marker)
{
	if (!Marker)
	{
		return;
	}

	// 중복 등록 방지 (OnRep과 BeginPlay가 둘 다 호출될 수 있음)
	if (Markers.Contains(Marker))
	{
		return;
	}

	Markers.Add(Marker);

	OnWaypointListChanged.Broadcast();
}

void UNSWaypointSubsystem::UnregisterMarker(UNSWaypointMarkerComponent* Marker)
{
	// 스테일 항목도 함께 정리하면서 대상 제거
	const int32 RemovedCount = Markers.RemoveAll(
		[Marker](const TWeakObjectPtr<UNSWaypointMarkerComponent>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Marker;
		});

	if (RemovedCount <= 0)
	{
		return;
	}

	OnWaypointListChanged.Broadcast();
}
