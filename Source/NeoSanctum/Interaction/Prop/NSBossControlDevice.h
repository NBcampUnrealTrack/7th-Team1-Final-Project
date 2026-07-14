// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSDestructibleObjectBase.h"
#include "NSBossControlDevice.generated.h"

class ANSBossControlDevice;

DECLARE_MULTICAST_DELEGATE_OneParam(FNSControlDeviceDestroyed, ANSBossControlDevice*);

UCLASS()
class NEOSANCTUM_API ANSBossControlDevice : public ANSDestructibleObjectBase
{
	GENERATED_BODY()

public:
	ANSBossControlDevice();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버에서 이 장치가 파괴될 때 1회 발생 (보스가 바인딩)
	FNSControlDeviceDestroyed OnControlDeviceDestroyed;

	// [서버 전용] MotherShip이 바닥 트레이스로 확정한 최종 위치/회전을 적용하고 복제 프로퍼티에 반영
	// 호출부(ANSBossMotherShip::PlaceControlDeviceOnGround)가 이미 HasAuthority() 가드 안에서만 호출.
	void ApplyGroundPlacement(const FVector& InGroundLocation, const FRotator& InGroundRotation);

	// 액터 피벗에서 메시 바운드 최하단까지의 거리. 바닥 Z에 더하면 메시가 바닥에 파묻히지 않음.
	float GetPivotToMeshBottomOffset() const;

protected:
	// 베이스 서버 파괴 훅 오버라이드 → 소유 보스에게 파괴 통지
	virtual void OnServerDestroyed(const FVector& Origin) override;

private:
	UFUNCTION()
	void OnRep_GroundPlaced();

	// 부착 해제 + 위치/회전 적용 (서버 직접 호출과 OnRep 양쪽에서 공유)
	void ApplyPlacementTransform();

	// ---- Replicated: 서버가 바닥 트레이스로 확정한 최종 배치값 ----
	// bReplicateMovement=false(NSDestructibleObjectBase)라 위치 변경이 자동 전파되지 않으므로,
	// 기존 ImpactAnchor 패턴과 동일하게 명시적 복제 프로퍼티 + OnRep으로 전파한다.
	UPROPERTY(ReplicatedUsing = OnRep_GroundPlaced)
	bool bGroundPlaced = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize GroundLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FRotator GroundRotation = FRotator::ZeroRotator;
};
