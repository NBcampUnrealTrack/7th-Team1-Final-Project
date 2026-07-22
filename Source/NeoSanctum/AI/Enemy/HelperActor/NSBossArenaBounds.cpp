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

FVector ANSBossArenaBounds::GetArenaCenter(float AtZ) const
{
	// 로컬 중심 = (0,0,0)
	const FVector LocalPoint(0.f, 0.f, 0.f);

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

FVector ANSBossArenaBounds::GetDiagonalStartCorner(float AtZ) const
{
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();
	
	const FVector LocalPoint(-HalfExtent.X, -HalfExtent.Y, 0.f);
	
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);
	WorldPoint.Z = AtZ;
	
	return WorldPoint;
}

FVector ANSBossArenaBounds::GetDiagonalEndCorner(float AtZ) const
{
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();
	
	const FVector LocalPoint(HalfExtent.X, HalfExtent.Y, 0.f);
	
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);
	WorldPoint.Z = AtZ;
	
	return WorldPoint;
}

FVector ANSBossArenaBounds::ClampPointToBounds(const FVector& WorldPoint) const
{
	// 박스의 Transform 을 가져옴
	const FTransform& BoxTransform = AreaBox->GetComponentTransform();
	// 박스의 언스케일 반경 가져옴
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();

	// 전달받은 보스의 현재위치가 현재 박스의 로컬기준으로 어디에 있는지 변환
	FVector LocalPoint = BoxTransform.InverseTransformPosition(WorldPoint);
	
	// 받은 점을 경계 안쪽으로 클램프
	LocalPoint.X = FMath::Clamp(LocalPoint.X, -HalfExtent.X, HalfExtent.X);
	LocalPoint.Y = FMath::Clamp(LocalPoint.Y, -HalfExtent.Y, HalfExtent.Y);

	// 적용된 로컬 좌표를 다시 월드좌표로 변환
	FVector ClampedWorldPoint = BoxTransform.TransformPosition(LocalPoint);
	
	// Z축은 월드기준으로 유지
	ClampedWorldPoint.Z = WorldPoint.Z;

	// 변환된 좌표를 반환
	return ClampedWorldPoint;
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

FVector ANSBossArenaBounds::GetSlotCenter(int32 ZoneIndex, int32 SlotIndex, int32 ZoneCount, int32 SlotCount) const
{
	// 박스 사이즈 가져오기
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();

	// 데칼 존 1개의 폭 구하기
	const float ZoneWidth = (2.f * HalfExtent.X) / ZoneCount;
	// 데칼 존 1개의 로컬 중심 좌표
	const float ZoneLocalX = -HalfExtent.X + (ZoneIndex + 0.5f) * ZoneWidth;
	
	// 데칼 존 1개 를 n등분한 슬롯n의 폭
	const float RoomWidth = 2.f * HalfExtent.Y;
	const float SlotWidth = RoomWidth / SlotCount;
	// 슬롯 n의 중심 Y좌표
	const float SlotLocalY = (SlotIndex - (SlotCount - 1) / 2.f) * SlotWidth;

	// 로컬 좌표 생성
	const FVector LocalPoint(ZoneLocalX, SlotLocalY, 0.f);

	// 월드 좌표 변환 후 반환
	FVector WorldPoint = AreaBox->GetComponentTransform().TransformPosition(LocalPoint);

	return WorldPoint;
}

FVector ANSBossArenaBounds::GetSlotBoxExtent(int32 ZoneCount, int32 SlotCount, float ZoneHeight) const
{
	const FVector HalfExtent = AreaBox->GetUnscaledBoxExtent();

	// 존 1개의 가로폭
	const float ZoneWidth = (2.f * HalfExtent.X) / ZoneCount;
	// 존 1개의 세로폭
	const float RoomWidth = 2.f * HalfExtent.Y;
	
	// 슬롯 하나의 세로폭
	const float SlotWidth = RoomWidth / SlotCount;

	const float SlotX = ZoneWidth / 2.f;
	const float SlotY = SlotWidth / 2.f;
	const float SlotZ = ZoneHeight / 2.f;

	return FVector(SlotX, SlotY, SlotZ);
}

void ANSBossArenaBounds::OverlapPlayersInSlot(int32 ZoneIndex, int32 SlotIndex, int32 ZoneCount, int32 SlotCount,
	float ZoneHeight, TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	const FVector SlotCenter = GetSlotCenter(ZoneIndex, SlotIndex, ZoneCount, SlotCount);
	const FVector SlotExtent = GetSlotBoxExtent(ZoneCount, SlotCount, ZoneHeight);
	const FQuat SlotRotation = GetZoneBoxRotation();

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		SlotCenter,
		SlotRotation,
		PlayerOverlapChannel,
		FCollisionShape::MakeBox(SlotExtent));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* OverlappedActor = Overlap.GetActor())
		{
			OutActors.AddUnique(OverlappedActor);
		}
	}
}


