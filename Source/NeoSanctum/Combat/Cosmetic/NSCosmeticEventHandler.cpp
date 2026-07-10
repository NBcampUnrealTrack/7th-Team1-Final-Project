// Copyright 2026 One Team. All rights reserved.


#include "NSCosmeticEventHandler.h"

#include "NSEnemyCosmeticComponent.h"

void UNSCosmeticEventHandler::Initialize(UNSEnemyCosmeticComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void UNSCosmeticEventHandler::GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const
{
}

void UNSCosmeticEventHandler::HandleEvent(
	AActor* OwnerActor,
	const FNSCosmeticEventNetData& EventData)
{
}

UWorld* UNSCosmeticEventHandler::GetWorld() const
{
	return OwnerComponent ? OwnerComponent->GetWorld() : nullptr;
}
