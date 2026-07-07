// Copyright 2026 One Team. All rights reserved.


#include "NSBossArenaBounds.h"

#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"


ANSBossArenaBounds::ANSBossArenaBounds()
{
	AreaBox = CreateDefaultSubobject<UBoxComponent>("ANSBossArenaBounds");
	SetRootComponent(AreaBox);
	
	PrimaryActorTick.bCanEverTick = false;
}

FVector ANSBossArenaBounds::GetFarEndCenter(float AtZ) const
{
	// 스케일은 TransformPosition이 적용하므로 로컬 포인트는 Unscaled 기준으로 구성
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();

	// 로컬 +X = 진행축(보스 쪽 → 입구 쪽) 규약상, 먼 쪽(보스 쪽)은 로컬 X 음수 극단
	const FVector LocalPoint(-HalfExtent.X, 0.f, 0.f);

	// 위치·회전·스케일을 한 번에 반영해 월드 좌표로 변환
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);

	// 고도는 룸 기하가 아니라 호출자(비행 로직)가 결정
	WorldPoint.Z = AtZ;

	return WorldPoint;
}

FVector ANSBossArenaBounds::GetEntranceCenter(float AtZ) const
{
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();
	
	const FVector LocalPoint(HalfExtent.X, 0.f, 0.f);
	
	// 위치·회전·스케일을 한 번에 반영해 월드 좌표로 변환
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);

	// 고도는 룸 기하가 아니라 호출자(비행 로직)가 결정
	WorldPoint.Z = AtZ;

	return WorldPoint;
}

FVector ANSBossArenaBounds::GetZoneCenter(int32 ZoneIndex, int32 ZoneCount) const
{
	// 시작지점 절반의 크기
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();
	
	// 존 i의 폭
	const float ZoneWidth = (2.f * HalfExtent.X) / ZoneCount;
	
	const float ZoneLocalX = -HalfExtent.X + (ZoneIndex + 0.5f) * ZoneWidth;
	const float ZoneLocalY = 0.f;
	
	const FVector LocalPoint(ZoneLocalX, ZoneLocalY, 0.f);
	
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);
	
	return WorldPoint;
}

FVector ANSBossArenaBounds::GetZoneBoxExtent(int32 ZoneCount, float ZoneHeight) const
{
	// 시작지점 절반의 크기
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();
	
	// 존 i의 폭
	const float ZoneWidth = (2.f * HalfExtent.X) / ZoneCount;
	
	const float ZoneX = ZoneWidth / 2.f;
	const float ZoneY = HalfExtent.Y;
	const float ZoneZ = ZoneHeight / 2.f;
	
	const FVector Extent(ZoneX, ZoneY, ZoneZ);
	
	return Extent;
}

FQuat ANSBossArenaBounds::GetZoneBoxRotation() const
{
	return AreaBox->GetComponentQuat();
}

void ANSBossArenaBounds::OverlapPlayersInZone(int32 ZoneIndex, int32 ZoneCount, float ZoneHeight,
	TArray<AActor*>& OutActors) const
{
	OutActors.Reset();
	
	const FVector ZoneCenter = GetZoneCenter(ZoneIndex, ZoneCount);
	const FVector ZoneExtent = GetZoneBoxExtent(ZoneCount, ZoneHeight);
	const FQuat ZoneRotation = GetZoneBoxRotation();
	
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		ZoneCenter,
		ZoneRotation,
		PlayerOverlapChannel,
		FCollisionShape::MakeBox(ZoneExtent));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* OverlappedActor = Overlap.GetActor())
		{
			// 플레이어 폰에 콜리전 컴포넌트가 여러 개면 같은 액터가 중복으로 걸릴 수 있어 AddUnique로 방지
			OutActors.AddUnique(OverlappedActor);
		}
	}
}


