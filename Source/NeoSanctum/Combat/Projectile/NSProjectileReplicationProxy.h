// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSProjectileTypes.h"
#include "NSProjectileReplicationProxy.generated.h"

class USceneComponent;
class UNSProjectileVisualManagerComponent;

/**
 * 각 PlayerController가 소유하는 Owner-only 네트워크 전달 Actor
 *
 * 서버 투사체 매니저는 PlayerController를 직접 수정하지 않고
 * 이 Actor를 통해 해당 플레이어에게 이벤트를 전달한다.
 */
UCLASS()
class NEOSANCTUM_API ANSProjectileReplicationProxy : public AActor
{
	GENERATED_BODY()

public:
	ANSProjectileReplicationProxy();

	// 서버가 이 Proxy의 소유 클라이언트에게 생성 이벤트를 보냄
	void SendSpawnEvent(const FNSProjectileSpawnEvent& SpawnEvent);

	// 서버가 이 Proxy의 소유 클라이언트에게 종료 이벤트를 보냄
	void SendEndEvent(const FNSProjectileEndEvent& EndEvent);

private:
	UFUNCTION(Client, Reliable)
	void Client_SpawnProjectile(const FNSProjectileSpawnEvent& SpawnEvent);

	UFUNCTION(Client, Reliable)
	void Client_EndProjectile(const FNSProjectileEndEvent& EndEvent);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UNSProjectileVisualManagerComponent> VisualManager;
};
