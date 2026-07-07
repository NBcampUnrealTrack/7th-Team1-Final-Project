// Copyright 2026 One Team. All rights reserved.


#include "NSHealReplicationProxy.h"
#include "Components/SceneComponent.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Progression/Heal/NSLocalHealPickup.h"

ANSHealReplicationProxy::ANSHealReplicationProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	
	// 위치가 의미없는 순수 데이터 채널이라 이동 복제가 필요없음
	bAlwaysRelevant = false;
	AActor::SetReplicateMovement(false);
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ANSHealReplicationProxy::SendSpawnEvent(const FNSHealSpawnEvent& Event)
{
	// 서버에서만 호출되도록
	if (!HasAuthority())
	{
		return;
	}
	Client_SpawnHeal(Event);
}

void ANSHealReplicationProxy::SendRemoveEvent(int32 DropId)
{
	// 서버에서만 호출되도록
	if (!HasAuthority())
	{
		return;
	}
	Client_RemoveHeal(DropId);
}

void ANSHealReplicationProxy::SendRestoreEvent(int32 DropId)
{
	if (!HasAuthority())
	{
		return;
	}
	Client_RestoreHeal(DropId);
}

void ANSHealReplicationProxy::BeginPlay()
{
	Super::BeginPlay();
	if (HealPotionTable.IsNull())
	{
		return;
	}

	if (UDataTable* AlreadyLoaded = HealPotionTable.Get())
	{
		LoadedHealPotionTable = AlreadyLoaded;
		return;
	}

	HealPotionTableLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		HealPotionTable.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([this]()
		{
			LoadedHealPotionTable = Cast<UDataTable>(HealPotionTable.ToSoftObjectPath().ResolveObject());
		})
	);
}

void ANSHealReplicationProxy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealPotionTableLoadHandle.IsValid())
	{
		HealPotionTableLoadHandle->CancelHandle();
		HealPotionTableLoadHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ANSHealReplicationProxy::Client_SpawnHeal_Implementation(const FNSHealSpawnEvent& Event)
{
	// BP에서 PickupClass를 지정하지 않으면 스폰할 대상이 없으므로 조용히 무시
	if (!PickupClass)
	{
		return;
	}
	
	// RegisterProxy(늦은 참가자 동기화)와 RegisterDrop(신규 드랍)이 둘 다 이 RPC를 호출할 수 있어 중복 방지
	if (ActivePickups.Contains(Event.DropId))
	{
		return;
	}
	
	// 포물선 발사 드랍
	const FVector SpawnLocation = Event.LaunchData.IsValid() ? FVector(Event.LaunchData.StartLocation) : FVector(Event.Location);
	const FTransform SpawnTM(FRotator::ZeroRotator, SpawnLocation);
	
	ANSLocalHealPickup* Pickup = GetWorld()->SpawnActorDeferred<ANSLocalHealPickup>(
		PickupClass, SpawnTM, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (!Pickup)
	{
		return;
	}
	
	Pickup->Initialize(Event, LoadedHealPotionTable);
	Pickup->FinishSpawning(SpawnTM);
	
	ActivePickups.Add(Event.DropId, Pickup);
}

void ANSHealReplicationProxy::Client_RemoveHeal_Implementation(int32 DropId)
{
	TObjectPtr<ANSLocalHealPickup> Pickup;
	// 맵에서 제거하면서 포인터 꺼내오기 -> 같은 DropId로 스폰요청이 와도 정상적으로 새 픽업 만들 수 있음
	if (ActivePickups.RemoveAndCopyValue(DropId, Pickup) && Pickup)
	{
		Pickup->ConfirmCollected();
	}
}

void ANSHealReplicationProxy::Client_RestoreHeal_Implementation(int32 DropId)
{
	// 숨겨진 비주얼을 복구하는것이므로 삭제가 없음
	if (TObjectPtr<ANSLocalHealPickup>* Found = ActivePickups.Find(DropId))
	{
		if (*Found)
		{
			(*Found)->RestoreVisual();
		}
	}
}
