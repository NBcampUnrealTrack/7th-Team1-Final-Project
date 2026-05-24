// Copyright 2026 One Team. All rights reserved.


#include "NSSoundSubsystem.h"

#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"

UNSSoundSubsystem* UNSSoundSubsystem::Get(const UObject* WorldContext)
{
	UNSGameInstance* GameInstance = UNSGameInstance::Get(WorldContext);
	if (!GameInstance)
	{
		return nullptr;
	}
	
	return GameInstance->GetSubsystem<UNSSoundSubsystem>();
}

void UNSSoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UNSSoundSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
