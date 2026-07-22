// Copyright 2026 One Team. All rights reserved.

#include "NSDroppedPartRegistrySubsystem.h"

#include "NeoSanctum/Progression/Part/NSDroppedPart.h"

void UNSDroppedPartRegistrySubsystem::RegisterDrop(ANSDroppedPart* Drop)
{
	if (Drop)
	{
		// 드롭이 유효하다면 약참조로 들고있음
		ActiveDrops.AddUnique(TWeakObjectPtr<ANSDroppedPart>(Drop));
	}
}

// 해당 드롭제거, 파츠 교체/디스폰 시
void UNSDroppedPartRegistrySubsystem::UnregisterDrop(ANSDroppedPart* Drop)
{
	ActiveDrops.RemoveSingleSwap(TWeakObjectPtr<ANSDroppedPart>(Drop));
}

// 기존 스폰된 파츠와 겹치는지 확인, 겹치면 true
bool UNSDroppedPartRegistrySubsystem::IsLocationOccupied(const FVector& Location, float AvoidRadius) const
{
	// 최소 겹침 범위
	const float RadiusSq = AvoidRadius * AvoidRadius;
	
	for (const TWeakObjectPtr<ANSDroppedPart>& WeakDrop : ActiveDrops)
	{
		const ANSDroppedPart* Drop = WeakDrop.Get();
		if (!Drop)
		{
			continue;
		}
		if (FVector::DistSquared(Drop->GetActorLocation(), Location) <= RadiusSq)
		{
			return true;
		}
	}
	return false;
}
