// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCosmeticComponent.h"

#include "NSCosmeticEventHandler.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogNSEnemyCosmetic, Log, All);

UNSEnemyCosmeticComponent::UNSEnemyCosmeticComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSEnemyCosmeticComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeHandlers();
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

	InitializeHandlers();

	if (UNSCosmeticEventHandler** Handler = HandlerMap.Find(EventData.EventTag))
	{
		(*Handler)->HandleEvent(GetOwner(), EventData);
		return;
	}

	if (EventData.Phase != ENSCosmeticEventPhase::Update)
	{
		UE_LOG(
			LogNSEnemyCosmetic,
			Warning,
			TEXT("CosmeticEvent Handler 없음. Owner=%s, EventTag=%s, HandlerClassCount=%d, RegisteredTagCount=%d"),
			*GetNameSafe(GetOwner()),
			*EventData.EventTag.ToString(),
			HandlerClasses.Num(),
			HandlerMap.Num());
	}
}

void UNSEnemyCosmeticComponent::InitializeHandlers()
{
	if (bHandlersInitialized)
	{
		return;
	}

	bHandlersInitialized = true;
	HandlerInstances.Reset();
	HandlerMap.Reset();

	UE_LOG(
		LogNSEnemyCosmetic,
		Log,
		TEXT("InitializeHandlers. Owner=%s, HandlerClassCount=%d"),
		*GetNameSafe(GetOwner()),
		HandlerClasses.Num());

	for (int32 Index = 0; Index < HandlerClasses.Num(); ++Index)
	{
		const TSubclassOf<UNSCosmeticEventHandler>& HandlerClass = HandlerClasses[Index];
		UClass* HandlerClassObject = HandlerClass.Get();

		if (!HandlerClassObject || HandlerClassObject->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(
				LogNSEnemyCosmetic,
				Warning,
				TEXT("HandlerClass invalid. Owner=%s, Index=%d, HandlerClass=%s"),
				*GetNameSafe(GetOwner()),
				Index,
				*GetNameSafe(HandlerClassObject));
			continue;
		}

		UNSCosmeticEventHandler* Handler =
			NewObject<UNSCosmeticEventHandler>(this, HandlerClassObject);

		if (!Handler)
		{
			continue;
		}

		Handler->Initialize(this);
		HandlerInstances.Add(Handler);
		RegisterHandler(Handler);
	}
}

void UNSEnemyCosmeticComponent::RegisterHandler(UNSCosmeticEventHandler* Handler)
{
	if (!Handler)
	{
		return;
	}

	TArray<FGameplayTag> EventTags;
	Handler->GetHandledEventTags(EventTags);

	UE_LOG(
		LogNSEnemyCosmetic,
		Log,
		TEXT("RegisterHandler. Owner=%s, Handler=%s, EventTagCount=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Handler),
		EventTags.Num());

	for (const FGameplayTag& EventTag : EventTags)
	{
		if (EventTag.IsValid())
		{
			HandlerMap.Add(EventTag, Handler);

			UE_LOG(
				LogNSEnemyCosmetic,
				Log,
				TEXT("Cosmetic Handler Registered. EventTag=%s, Handler=%s"),
				*EventTag.ToString(),
				*GetNameSafe(Handler));
		}
	}
}
