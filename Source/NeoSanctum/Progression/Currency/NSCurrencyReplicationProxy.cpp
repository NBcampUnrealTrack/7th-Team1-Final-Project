// Copyright 2026 One Team. All rights reserved.

#include "NSCurrencyReplicationProxy.h"
#include "Components/SceneComponent.h"
#include "NeoSanctum/Progression/Currency/NSLocalCurrencyPickup.h"

ANSCurrencyReplicationProxy::ANSCurrencyReplicationProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Client RPC 전달을 위해 소유 클라에만 복제
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	bAlwaysRelevant = false;
	AActor::SetReplicateMovement(false);
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ANSCurrencyReplicationProxy::SendSpawnEvent(const FNSCurrencySpawnEvent& Event)
{
	if (!HasAuthority())
	{
		return;
	}
	Client_SpawnCurrency(Event);
}

void ANSCurrencyReplicationProxy::SendRemoveEvent(int32 DropId)
{
	if (!HasAuthority())
	{
		return;
	}
	Client_RemoveCurrency(DropId);
}

void ANSCurrencyReplicationProxy::SendRestoreEvent(int32 DropId)
{
	if (!HasAuthority())
	{
		return;
	}
	Client_RestoreCurrency(DropId);
}

void ANSCurrencyReplicationProxy::Client_SpawnCurrency_Implementation(const FNSCurrencySpawnEvent& Event)
{
	if (!PickupClass)
	{
		return;
	}
	// 중복 스폰 방지용
	if (ActivePickups.Contains(Event.DropId))
	{
		return;
	}
	
	const FTransform SpawnTM(FRotator::ZeroRotator, Event.Location);
	
	ANSLocalCurrencyPickup* Pickup = GetWorld()->SpawnActorDeferred<ANSLocalCurrencyPickup>(
		PickupClass, SpawnTM, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
	{
		return;
	}	
	Pickup->Initialize(Event, VisualData);
	Pickup->FinishSpawning(SpawnTM);
	
	ActivePickups.Add(Event.DropId, Pickup);
}

void ANSCurrencyReplicationProxy::Client_RemoveCurrency_Implementation(int32 DropId)
{
	TObjectPtr<ANSLocalCurrencyPickup> Pickup;
	if (ActivePickups.RemoveAndCopyValue(DropId, Pickup) && Pickup)
	{
		Pickup->ConfirmCollected();
	}
}

void ANSCurrencyReplicationProxy::Client_RestoreCurrency_Implementation(int32 DropId)
{
	if (TObjectPtr<ANSLocalCurrencyPickup>* Found = ActivePickups.Find(DropId))
	{
		if (*Found)
		{
			(*Found)->RestoreVisual();
		}
	}
}
