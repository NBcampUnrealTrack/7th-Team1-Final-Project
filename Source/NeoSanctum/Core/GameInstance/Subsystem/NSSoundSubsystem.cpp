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
	
	if (const UNSGameInstance* GameInstance = UNSGameInstance::Get(GetWorld()))
	{
		SoundDataTable = GameInstance->SoundDataTable;
	}
}

void UNSSoundSubsystem::Deinitialize()
{
	SoundDataTable = nullptr;
	
	Super::Deinitialize();
}

void UNSSoundSubsystem::PlayBGM(FName SoundID, float FadeIn)
{
}

void UNSSoundSubsystem::PlaySound2D(FName SoundID)
{
}

void UNSSoundSubsystem::PlaySoundAtLocation(FName SoundID, FVector Location)
{
}

void UNSSoundSubsystem::PlaySoundAttached(FName SoundID, USceneComponent* AttachToComponent, FName SocketName)
{
}

void UNSSoundSubsystem::StopBGM(float FadeOut)
{
}

void UNSSoundSubsystem::StopSound(FName SoundID, float FadeOut)
{
}

void UNSSoundSubsystem::StopCategory(ENSSoundCategory Category, float FadeOut)
{
}
