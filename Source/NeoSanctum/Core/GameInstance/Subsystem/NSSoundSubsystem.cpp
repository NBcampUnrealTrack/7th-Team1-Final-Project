// Copyright 2026 One Team. All rights reserved.


#include "NSSoundSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"
#include "NeoSanctum/Data/Sound/NSSoundData.h"

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
		SoundData = GameInstance->SoundData;
	}
	
	if (!SoundData)
	{
		SoundData = NewObject<UNSSoundData>(this);
	}
}

void UNSSoundSubsystem::Deinitialize()
{
	SoundData = nullptr;

	Super::Deinitialize();
}

UAudioComponent* UNSSoundSubsystem::PlayBGM(FName SoundID, float FadeIn)
{
}

UAudioComponent* UNSSoundSubsystem::PlaySound2D(FName SoundID)
{
}

UAudioComponent* UNSSoundSubsystem::PlaySoundAtLocation(FName SoundID, FVector Location)
{
}

UAudioComponent* UNSSoundSubsystem::PlaySoundAttached(FName SoundID, USceneComponent* AttachToComponent, FName SocketName)
{
}

void UNSSoundSubsystem::StopBGM(float FadeOut)
{
}

void UNSSoundSubsystem::StopCategory(ENSSoundCategory Category, float FadeOut)
{
}

void UNSSoundSubsystem::SetMasterVolume(float Volume)
{
}

void UNSSoundSubsystem::SetCategoryVolume(ENSSoundCategory Category, float Volume)
{
}

float UNSSoundSubsystem::GetCategoryVolume(ENSSoundCategory Category) const
{
}

const FNSSoundDataTableRow* UNSSoundSubsystem::FindSoundRow(FName SoundID) const
{
	
}

void UNSSoundSubsystem::PlayOneShot2D(const FNSSoundDataTableRow& SoundRow) const
{
}

void UNSSoundSubsystem::PlayLoop2D(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	float FadeIn
)
{
}

UAudioComponent* UNSSoundSubsystem::PlayLoopAtLocation(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	FVector Location
)
{
}

UAudioComponent* UNSSoundSubsystem::PlayLoopAttached(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	USceneComponent* AttachToComponent,
	FName SocketName
)
{
}

void UNSSoundSubsystem::RegisterLoop(
	UAudioComponent* Component,
	FName SoundID, ENSSoundCategory Category,
	ENSActiveSoundMode Mode, FVector Location,
	USceneComponent* AttachToComponent,
	FName SocketName
)
{
}

void UNSSoundSubsystem::OnLoopFinished(UAudioComponent* FinishedComponent)
{
}

void UNSSoundSubsystem::StopLoopsBySoundID(FName SoundID, float FadeOut)
{
}

void UNSSoundSubsystem::StopAllLoops(float FadeOut)
{
}

float UNSSoundSubsystem::GetFinalVolume(const FNSSoundDataTableRow& SoundRow) const
{
}


void UNSSoundSubsystem::InitializeCategoryVolumes()
{
}

void UNSSoundSubsystem::ApplyVolume(ENSSoundCategory Category)
{
	FNSActiveSound* ActiveSound = ActiveSounds.Find(Category);
	if (!ActiveSound || !ActiveSound->Component || ActiveSound->SoundID.IsNone())
	{
		return;
	}

	const FNSSoundDataTableRow* SoundRow = FindSoundRow(ActiveSound->SoundID);
	if (!SoundRow)
	{
		return;
	}

	ActiveSound->Component->SetVolumeMultiplier(GetFinalVolume(*SoundRow));
}
