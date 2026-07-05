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

public:
	// 서버에서 이 장치가 파괴될 때 1회 발생 (보스가 바인딩)
	FNSControlDeviceDestroyed OnControlDeviceDestroyed;

protected:
	// 베이스 서버 파괴 훅 오버라이드 → 소유 보스에게 파괴 통지
	virtual void OnServerDestroyed(const FVector& Origin) override;
};
