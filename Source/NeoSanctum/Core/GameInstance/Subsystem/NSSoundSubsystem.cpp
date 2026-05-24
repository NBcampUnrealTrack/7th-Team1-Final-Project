// Copyright 2026 One Team. All rights reserved.


#include "NSSoundSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"
#include "NeoSanctum/Data/Sound/SoundData.h"

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
	// 현재 재생되는 BGM이 있고 그 BGM이 재생을 요청한 SoundID와 같다면 중복재생을 방지하기 위해 return
	if (CurrentBGM && CurrentBGMID == SoundID)
	{
		return;
	}
	
	const FNSSoundDataTableRow* SoundRow = FindSoundRow(SoundID);
	if (!SoundRow || !SoundRow->Sound)
	{
		return;
	}
	
	// 기존에 재생되던 BGM 중지
	StopBGM(SoundRow->FadeOutTime);
	
	CurrentBGM = UGameplayStatics::SpawnSound2D(
		GetGameInstance(),
		SoundRow->Sound,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		nullptr,
		true
	);
	
	if (CurrentBGM)
	{
		CurrentBGMID = SoundID;
	}
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
	if (!CurrentBGM)
	{
		CurrentBGMID = NAME_None;
		return;
	}
	
	const float FadeOutTime = FMath::Max(0.f, FadeOut);
	CurrentBGM->OnAudioFinished.RemoveAll(this);
	if (FadeOutTime > 0.f)
	{
		CurrentBGM->FadeOut(FadeOutTime, 0.f);
	}
	else
	{
		CurrentBGM->Stop();
	}
	
	CurrentBGM = nullptr;
	CurrentBGMID = NAME_None;
}

void UNSSoundSubsystem::StopSound(FName SoundID, float FadeOut)
{
}

void UNSSoundSubsystem::StopCategory(ENSSoundCategory Category, float FadeOut)
{
}

void UNSSoundSubsystem::SetMasterVolume(float Volume)
{
}

float UNSSoundSubsystem::GetCategoryVolume(ENSSoundCategory Category) const
{
	switch (Category)
	{
	case ENSSoundCategory::BGM:
		return BGMVolume;
	case ENSSoundCategory::SFX:
		return SFXVolume;
	case ENSSoundCategory::UI:
		return UIVolume;
	default:
		return 1.f;
	}
}

const FNSSoundDataTableRow* UNSSoundSubsystem::FindSoundRow(FName SoundID) const
{
	if (!SoundDataTable || SoundID.IsNone())
	{
		return nullptr;
	}
	
	return SoundDataTable->FindRow<FNSSoundDataTableRow>(SoundID, TEXT("NSSoundSubsystem"));
}

float UNSSoundSubsystem::GetFinalVolume(const FNSSoundDataTableRow& SoundRow) const
{
	return SoundRow.Volume * MasterVolume * GetCategoryVolume(SoundRow.Category);
}
