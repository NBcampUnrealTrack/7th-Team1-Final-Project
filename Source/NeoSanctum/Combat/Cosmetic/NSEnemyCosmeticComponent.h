// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"
#include "NSEnemyCosmeticComponent.generated.h"

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyCosmeticComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyCosmeticComponent();

	// 서버에서 지속형 코스메틱 식별자를 발급하는 함수
	int32 AllocateCosmeticInstanceId();

	// 서버에서 코스메틱 이벤트를 Reliable 또는 Unreliable로 전송하는 함수
	void SendCosmeticEvent(const FNSCosmeticEventNetData& EventData, bool bReliable);

	// 고빈도 코스메틱 이벤트를 모든 클라이언트에 전달하는 함수
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayCosmeticEvent(const FNSCosmeticEventNetData& EventData);

	// 놓치면 안 되는 코스메틱 이벤트를 모든 클라이언트에 전달하는 함수
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayImportantCosmeticEvent(const FNSCosmeticEventNetData& EventData);

private:
	// 클라이언트에서 수신한 코스메틱 이벤트를 처리하는 함수
	void HandleCosmeticEvent_Client(const FNSCosmeticEventNetData& EventData);

	// 서버에서 다음 코스메틱 인스턴스 ID를 저장하는 변수
	int32 NextCosmeticInstanceId = 1;
};
