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

	InitializeCategoryVolumes();
}

void UNSSoundSubsystem::Deinitialize()
{
	StopAllLoops(0.f);
	SoundData = nullptr;

	Super::Deinitialize();
}

UAudioComponent* UNSSoundSubsystem::PlayBGM(FName SoundID, float FadeIn)
{
	return PlaySound2D(SoundID, FadeIn);
}

UAudioComponent* UNSSoundSubsystem::PlaySound2D(FName SoundID, float FadeIn)
{
	const FNSSoundDataTableRow* SoundRow = FindSoundRow(SoundID);
	if (!SoundRow || !SoundRow->Sound)
	{
		return nullptr;
	}

	if (SoundRow->bLoop)
	{
		return PlayLoop2D(SoundID, *SoundRow, FadeIn);
	}

	PlayOneShot2D(*SoundRow);
	return nullptr;
}

UAudioComponent* UNSSoundSubsystem::PlaySoundAtLocation(FName SoundID, FVector Location)
{
	const FNSSoundDataTableRow* SoundRow = FindSoundRow(SoundID);
	if (!SoundRow || !SoundRow->Sound)
	{
		return nullptr;
	}

	if (SoundRow->bLoop)
	{
		return PlayLoopAtLocation(SoundID, *SoundRow, Location);
	}

	UGameplayStatics::PlaySoundAtLocation(
		GetGameInstance(),
		SoundRow->Sound,
		Location,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		SoundRow->Attenuation,
		SoundRow->Concurrency
	);

	return nullptr;
}

UAudioComponent* UNSSoundSubsystem::PlaySoundAttached(
	FName SoundID,
	USceneComponent* AttachToComponent,
	FName SocketName
)
{
	const FNSSoundDataTableRow* SoundRow = FindSoundRow(SoundID);
	if (!SoundRow || !SoundRow->Sound || !AttachToComponent)
	{
		return nullptr;
	}

	if (SoundRow->bLoop)
	{
		return PlayLoopAttached(SoundID, *SoundRow, AttachToComponent, SocketName);
	}

	return UGameplayStatics::SpawnSoundAttached(
		SoundRow->Sound,
		AttachToComponent,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		SoundRow->Attenuation,
		SoundRow->Concurrency,
		true
	);
}

void UNSSoundSubsystem::StopBGM(float FadeOut)
{
	StopCategory(ENSSoundCategory::BGM, FadeOut);
}

void UNSSoundSubsystem::StopSound(UAudioComponent* AudioComponent, float FadeOut)
{
	if (!AudioComponent)
	{
		return;
	}

	AudioComponent->OnAudioFinished.RemoveAll(this);
	AudioComponent->OnAudioFinishedNative.RemoveAll(this);

	const float FadeOutTime = FMath::Max(0.f, FadeOut);
	if (FadeOutTime > 0.f)
	{
		AudioComponent->FadeOut(FadeOutTime, 0.f);
	}
	else
	{
		AudioComponent->Stop();
	}

	ActiveSounds.RemoveAll([AudioComponent](const FNSActiveSound& ActiveSound)
	{
		return ActiveSound.Component.Get() == AudioComponent;
	});
}

void UNSSoundSubsystem::StopCategory(ENSSoundCategory Category, float FadeOut)
{
	TArray<UAudioComponent*> ComponentsToStop;
	for (const FNSActiveSound& ActiveSound : ActiveSounds)
	{
		if (ActiveSound.Category == Category && ActiveSound.Component)
		{
			ComponentsToStop.Add(ActiveSound.Component);
		}
	}

	for (UAudioComponent* Component : ComponentsToStop)
	{
		StopSound(Component, FadeOut);
	}
}

void UNSSoundSubsystem::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);

	TSet<ENSSoundCategory> AppliedCategories;
	for (const FNSActiveSound& ActiveSound : ActiveSounds)
	{
		if (AppliedCategories.Contains(ActiveSound.Category))
		{
			continue;
		}

		ApplyVolume(ActiveSound.Category);
		AppliedCategories.Add(ActiveSound.Category);
	}
}

void UNSSoundSubsystem::SetCategoryVolume(ENSSoundCategory Category, float Volume)
{
	CategoryVolumes.FindOrAdd(Category) = FMath::Clamp(Volume, 0.f, 1.f);
	ApplyVolume(Category);
}

float UNSSoundSubsystem::GetCategoryVolume(ENSSoundCategory Category) const
{
	const float* Volume = CategoryVolumes.Find(Category);
	return Volume ? *Volume : 1.f;
}

const FNSSoundDataTableRow* UNSSoundSubsystem::FindSoundRow(FName SoundID) const
{
	if (!SoundData || !SoundData->SoundDataTable || SoundID.IsNone())
	{
		return nullptr;
	}

	return SoundData->SoundDataTable->FindRow<FNSSoundDataTableRow>(SoundID, TEXT("NSSoundSubsystem"));
}

void UNSSoundSubsystem::PlayOneShot2D(const FNSSoundDataTableRow& SoundRow) const
{
	UGameplayStatics::PlaySound2D(
		GetGameInstance(),
		SoundRow.Sound,
		GetFinalVolume(SoundRow),
		SoundRow.Pitch,
		SoundRow.StartTime,
		SoundRow.Concurrency
	);
}

UAudioComponent* UNSSoundSubsystem::PlayLoop2D(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	float FadeIn
)
{
	if (!SoundRow.bAllowMultiple)
	{
		StopSoundLoop(SoundID, SoundRow.FadeOutTime);
	}

	UAudioComponent* NewComponent = UGameplayStatics::SpawnSound2D(
		GetGameInstance(),
		SoundRow.Sound,
		GetFinalVolume(SoundRow),
		SoundRow.Pitch,
		SoundRow.StartTime,
		nullptr,
		true
	);

	if (!NewComponent)
	{
		return nullptr;
	}

	RegisterLoop(NewComponent, SoundID, SoundRow.Category, ENSActiveSoundMode::TwoDimension);

	const float FadeInTime = FadeIn >= 0.f ? FadeIn : SoundRow.FadeInTime;
	if (FadeInTime > 0.f)
	{
		NewComponent->FadeIn(FadeInTime, GetFinalVolume(SoundRow), SoundRow.StartTime);
	}

	return NewComponent;
}

UAudioComponent* UNSSoundSubsystem::PlayLoopAtLocation(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	FVector Location
)
{
	if (!SoundRow.bAllowMultiple)
	{
		StopSoundLoop(SoundID, SoundRow.FadeOutTime);
	}

	UAudioComponent* NewComponent = UGameplayStatics::SpawnSoundAtLocation(
		GetGameInstance(),
		SoundRow.Sound,
		Location,
		FRotator::ZeroRotator,
		GetFinalVolume(SoundRow),
		SoundRow.Pitch,
		SoundRow.StartTime,
		SoundRow.Attenuation,
		SoundRow.Concurrency,
		true
	);

	RegisterLoop(NewComponent, SoundID, SoundRow.Category, ENSActiveSoundMode::AtLocation, Location);
	return NewComponent;
}

UAudioComponent* UNSSoundSubsystem::PlayLoopAttached(
	FName SoundID,
	const FNSSoundDataTableRow& SoundRow,
	USceneComponent* AttachToComponent,
	FName SocketName
)
{
	if (!SoundRow.bAllowMultiple)
	{
		StopSoundLoop(SoundID, SoundRow.FadeOutTime);
	}

	UAudioComponent* NewComponent = UGameplayStatics::SpawnSoundAttached(
		SoundRow.Sound,
		AttachToComponent,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		GetFinalVolume(SoundRow),
		SoundRow.Pitch,
		SoundRow.StartTime,
		SoundRow.Attenuation,
		SoundRow.Concurrency,
		true
	);

	RegisterLoop(NewComponent, SoundID, SoundRow.Category, ENSActiveSoundMode::Attached, FVector::ZeroVector,
	             AttachToComponent, SocketName);
	return NewComponent;
}

void UNSSoundSubsystem::RegisterLoop(
	UAudioComponent* Component,
	FName SoundID, ENSSoundCategory Category,
	ENSActiveSoundMode Mode, FVector Location,
	USceneComponent* AttachToComponent,
	FName SocketName
)
{
	if (!Component || SoundID.IsNone())
	{
		return;
	}

	FNSActiveSound& ActiveSound = ActiveSounds.AddDefaulted_GetRef();
	ActiveSound.Component = Component;
	ActiveSound.SoundID = SoundID;
	ActiveSound.Category = Category;
	ActiveSound.Mode = Mode;
	ActiveSound.Location = Location;
	ActiveSound.AttachComponent = AttachToComponent;
	ActiveSound.SocketName = SocketName;
	Component->OnAudioFinishedNative.AddUObject(this, &UNSSoundSubsystem::OnLoopFinished);
}

void UNSSoundSubsystem::OnLoopFinished(UAudioComponent* FinishedComponent)
{
	const int32 ActiveSoundIndex = ActiveSounds.IndexOfByPredicate(
		[FinishedComponent](const FNSActiveSound& ActiveSound)
		{
			return ActiveSound.Component.Get() == FinishedComponent;
		});

	if (ActiveSoundIndex == INDEX_NONE)
	{
		return;
	}

	const FNSActiveSound FinishedSound = ActiveSounds[ActiveSoundIndex];
	ActiveSounds.RemoveAtSwap(ActiveSoundIndex);

	const FNSSoundDataTableRow* SoundRow = FindSoundRow(FinishedSound.SoundID);
	if (!SoundRow || !SoundRow->Sound || !SoundRow->bLoop)
	{
		return;
	}

	switch (FinishedSound.Mode)
	{
	case ENSActiveSoundMode::TwoDimension:
		PlayLoop2D(FinishedSound.SoundID, *SoundRow, 0.f);
		break;
	case ENSActiveSoundMode::AtLocation:
		PlayLoopAtLocation(FinishedSound.SoundID, *SoundRow, FinishedSound.Location);
		break;
	case ENSActiveSoundMode::Attached:
		if (USceneComponent* AttachComponent = FinishedSound.AttachComponent.Get())
		{
			PlayLoopAttached(FinishedSound.SoundID, *SoundRow, AttachComponent, FinishedSound.SocketName);
		}
		break;
	default:
		break;
	}
}

void UNSSoundSubsystem::StopSoundLoop(FName SoundID, float FadeOut)
{
	TArray<UAudioComponent*> ComponentsToStop;
	for (const FNSActiveSound& ActiveSound : ActiveSounds)
	{
		if (ActiveSound.SoundID == SoundID && ActiveSound.Component)
		{
			ComponentsToStop.Add(ActiveSound.Component);
		}
	}

	for (UAudioComponent* Component : ComponentsToStop)
	{
		StopSound(Component, FadeOut);
	}
}

void UNSSoundSubsystem::StopAllLoops(float FadeOut)
{
	TArray<UAudioComponent*> ComponentsToStop;
	for (const FNSActiveSound& ActiveSound : ActiveSounds)
	{
		if (ActiveSound.Component)
		{
			ComponentsToStop.Add(ActiveSound.Component);
		}
	}

	for (UAudioComponent* Component : ComponentsToStop)
	{
		StopSound(Component, FadeOut);
	}
}

float UNSSoundSubsystem::GetFinalVolume(const FNSSoundDataTableRow& SoundRow) const
{
	return SoundRow.Volume * MasterVolume * GetCategoryVolume(SoundRow.Category);
}


void UNSSoundSubsystem::InitializeCategoryVolumes()
{
	CategoryVolumes.Reset();
	if (!SoundData)
	{
		return;
	}

	for (const FNSSoundCategorySettings& Settings : SoundData->CategorySettings)
	{
		CategoryVolumes.Add(Settings.Category, FMath::Clamp(Settings.DefaultVolume, 0.f, 1.f));
	}
}

void UNSSoundSubsystem::ApplyVolume(ENSSoundCategory Category)
{
	for (const FNSActiveSound& ActiveSound : ActiveSounds)
	{
		UAudioComponent* Component = ActiveSound.Component.Get();
		if (ActiveSound.Category != Category || !Component || ActiveSound.SoundID.IsNone())
		{
			continue;
		}

		const FNSSoundDataTableRow* SoundRow = FindSoundRow(ActiveSound.SoundID);
		if (!SoundRow)
		{
			continue;
		}

		Component->SetVolumeMultiplier(GetFinalVolume(*SoundRow));
	}
}
