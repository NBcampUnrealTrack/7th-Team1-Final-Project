// Copyright 2026 One Team. All rights reserved.


#include "NSBossControlDevice.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ANSBossControlDevice::ANSBossControlDevice()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANSBossControlDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSBossControlDevice, bGroundPlaced);
	DOREPLIFETIME(ANSBossControlDevice, GroundLocation);
	DOREPLIFETIME(ANSBossControlDevice, GroundRotation);
}

void ANSBossControlDevice::OnServerDestroyed(const FVector& Origin)
{
	Super::OnServerDestroyed(Origin);
	
	OnControlDeviceDestroyed.Broadcast(this);
}

void ANSBossControlDevice::ApplyGroundPlacement(const FVector& InGroundLocation, const FRotator& InGroundRotation)
{
	// 이미 파괴된 장치는 재배치하지 않음 (파괴 흐름과의 경합 방지)
	if (bDestroyed) return;

	GroundLocation = InGroundLocation;
	GroundRotation = InGroundRotation;
	bGroundPlaced = true;

	// 호스트(리슨 서버) 본인 화면 즉시 반영. 원격 클라는 OnRep_GroundPlaced가 담당.
	ApplyPlacementTransform();
}

float ANSBossControlDevice::GetPivotToMeshBottomOffset() const
{
	if (!StaticMeshComp) return 0.f;

	const float BoundsBottomZ = StaticMeshComp->Bounds.Origin.Z - StaticMeshComp->Bounds.BoxExtent.Z;
	return GetActorLocation().Z - BoundsBottomZ;
}

void ANSBossControlDevice::OnRep_GroundPlaced()
{
	if (!bGroundPlaced) return;

	// GroundLocation/GroundRotation은 bGroundPlaced와 같은 서버 프레임에 세팅되므로,
	// bDestroyed 처리와 동일하게 이 RepNotify 시점엔 두 값이 이미 반영돼 있다고 신뢰할 수 있음.
	ApplyPlacementTransform();
}

void ANSBossControlDevice::ApplyPlacementTransform()
{
	// 도착 순서 방어: 소켓 디태치의 AttachmentReplication과 이 프로퍼티의 도착 순서는 보장되지 않는다.
	// 아직 부모에 붙어있는 상태로 이 함수가 먼저 불려도, 여기서 한 번 더 디태치를 보장해
	// 최종적으로 항상 "비부착 + 바닥 위치" 상태로 수렴시킨다.
	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	SetActorLocationAndRotation(FVector(GroundLocation), GroundRotation);
}