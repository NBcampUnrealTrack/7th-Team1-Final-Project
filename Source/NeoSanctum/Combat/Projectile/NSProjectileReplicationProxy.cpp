// Copyright 2026 One Team. All rights reserved.

#include "NSProjectileReplicationProxy.h"
#include "NSProjectileVisualManagerComponent.h"
#include "Components/SceneComponent.h"

ANSProjectileReplicationProxy::ANSProjectileReplicationProxy()
{
	PrimaryActorTick.bCanEverTick = false;

	// Proxy Actor를 서버에서 클라이언트로 복제해야 
	// 이 Actor를 대상으로 하는 Client RPC를 전달할 수 있다.
	bReplicates = true;

	// Proxy 생성 시 Owner를 특정 PlayerController로 설정해 
	// 해당 PlayerController의 네트워크 연결에만 Proxy를 복제한다. (사본 복제 방지)
	bOnlyRelevantToOwner = true;

	// 모든 클라이언트에 항상 복제되는 Actor가 아님을 명시
	bAlwaysRelevant = false;

	// RPC 전달 통로이므로 위치, 회전, 속도 복제 필요 X
	AActor::SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualManager = CreateDefaultSubobject<UNSProjectileVisualManagerComponent>(
		TEXT("ProjectileVisualManagerComponent"));
}

void ANSProjectileReplicationProxy::SendSpawnEvent(const FNSProjectileSpawnEvent& SpawnEvent)
{
	if (!HasAuthority())
	{
		return;
	}

	Client_SpawnProjectile(SpawnEvent);
}

void ANSProjectileReplicationProxy::SendEndEvent(const FNSProjectileEndEvent& EndEvent)
{
	if (!HasAuthority())
	{
		return;
	}

	Client_EndProjectile(EndEvent);
}

void ANSProjectileReplicationProxy::Client_SpawnProjectile_Implementation(const FNSProjectileSpawnEvent& SpawnEvent)
{
	if (VisualManager)
	{
		VisualManager->HandleSpawnEvent(SpawnEvent);
	}
}

void ANSProjectileReplicationProxy::Client_EndProjectile_Implementation(const FNSProjectileEndEvent& EndEvent)
{
	if (VisualManager)
	{
		VisualManager->HandleEndEvent(EndEvent);
	}
}
