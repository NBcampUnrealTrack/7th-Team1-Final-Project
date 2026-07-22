// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Collision/NSCollisionChannels.h"
#include "NSBossArenaBounds.generated.h"

class UBoxComponent;

UCLASS()
class NEOSANCTUM_API ANSBossArenaBounds : public AActor
{
	GENERATED_BODY()

public:
	ANSBossArenaBounds();

	// 진행축 최상단(보스 시작 쪽) 중심을 반환. AtZ로 원하는 고도를 지정
	FVector GetFarEndCenter(float AtZ) const;

	// 진행축 입구 쪽 중심을 반환. AtZ로 원하는 고도를 지정
	FVector GetEntranceCenter(float AtZ) const;
	
	// 룸의 로컬 중심을 반환. AtZ로 원하는 고도를 지정
	FVector GetArenaCenter(float AtZ) const;

	// 존 i(0=보스 쪽, N-1=입구 쪽)의 월드 중심을 반환
	FVector GetZoneCenter(int32 ZoneIndex, int32 ZoneCount) const;

	// 존 박스 1개의 half-extent (X=진행축 폭/N, Y=룸 폭, Z=ZoneHeight/2)
	FVector GetZoneBoxExtent(int32 ZoneCount, float ZoneHeight) const;

	// 존 박스 판정에 사용할 회전 (액터 회전과 동일)
	FQuat GetZoneBoxRotation() const;

	// 대각선 시작 쪽(보스 쪽) 코너 중심을 반환. AtZ로 원하는 고도를 지정
	FVector GetDiagonalStartCorner(float AtZ) const;

	// 대각선 반대쪽 코너 중심을 반환. AtZ로 원하는 고도를 지정
	FVector GetDiagonalEndCorner(float AtZ) const;
	
	// 임의의 월드 좌표를 룸 XY 범위 안으로 클램프 (Z는 그대로 유지)
	FVector ClampPointToBounds(const FVector& WorldPoint) const;
	
	// 존 i 박스와 겹치는 플레이어 액터를 수집
	void OverlapPlayersInZone(int32 ZoneIndex, int32 ZoneCount, float ZoneHeight, TArray<AActor*>& OutActors) const;

	// 존 i 안에서 슬롯 s(0=한쪽 끝, M-1=반대쪽 끝)의 월드 중심을 반환
	FVector GetSlotCenter(int32 ZoneIndex, int32 SlotIndex, int32 ZoneCount, int32 SlotCount) const;

	// 슬롯 박스 1개의 half-extent (X=존폭/N, Y=룸폭/M, Z=ZoneHeight/2)
	FVector GetSlotBoxExtent(int32 ZoneCount, int32 SlotCount, float ZoneHeight) const;

	// 슬롯 박스와 겹치는 플레이어 액터를 수집
	void OverlapPlayersInSlot(int32 ZoneIndex, int32 SlotIndex,
		int32 ZoneCount, int32 SlotCount, float ZoneHeight, TArray<AActor*>& OutActors) const;
	
private:
	// 보스룸 사각형을 정의하는 루트 컴포넌트. 디자이너가 룸에 맞춰 배치
	UPROPERTY(VisibleAnywhere, Category = "ArenaBounds")
	TObjectPtr<UBoxComponent> AreaBox;

	// 존 오버랩 판정에 사용할 트레이스 채널 (기본: Player)
	UPROPERTY(EditDefaultsOnly, Category = "ArenaBounds")
	TEnumAsByte<ECollisionChannel> PlayerOverlapChannel = NSCollisionChannels::Player;
};
