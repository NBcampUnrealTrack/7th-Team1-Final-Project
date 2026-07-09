// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCosmeticComponent.h"

#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogNSEnemyCosmetic, Log, All);

UNSEnemyCosmeticComponent::UNSEnemyCosmeticComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

int32 UNSEnemyCosmeticComponent::AllocateCosmeticInstanceId()
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return INDEX_NONE;
	}

	return NextCosmeticInstanceId++;
}

void UNSEnemyCosmeticComponent::SendCosmeticEvent(
	const FNSCosmeticEventNetData& EventData,
	bool bReliable)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !EventData.EventTag.IsValid())
	{
		return;
	}

	if (bReliable)
	{
		Multicast_PlayImportantCosmeticEvent(EventData);
		return;
	}

	Multicast_PlayCosmeticEvent(EventData);
}

void UNSEnemyCosmeticComponent::Multicast_PlayCosmeticEvent_Implementation(
	const FNSCosmeticEventNetData& EventData)
{
	HandleCosmeticEvent_Client(EventData);
}

void UNSEnemyCosmeticComponent::Multicast_PlayImportantCosmeticEvent_Implementation(
	const FNSCosmeticEventNetData& EventData)
{
	HandleCosmeticEvent_Client(EventData);
}

void UNSEnemyCosmeticComponent::HandleCosmeticEvent_Client(
	const FNSCosmeticEventNetData& EventData)
{
	if (!EventData.EventTag.IsValid())
	{
		return;
	}

	UE_LOG(
		LogNSEnemyCosmetic,
		Log,
		TEXT("CosmeticEvent Received. Owner=%s, EventTag=%s, Phase=%d, InstanceId=%d, Location=%s"),
		*GetNameSafe(GetOwner()),
		*EventData.EventTag.ToString(),
		static_cast<uint8>(EventData.Phase),
		EventData.InstanceId,
		*EventData.Location.ToCompactString());
}
