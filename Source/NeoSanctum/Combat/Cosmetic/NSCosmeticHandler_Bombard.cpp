// Copyright 2026 One Team. All rights reserved.


#include "NSCosmeticHandler_Bombard.h"

#include "Components/SceneComponent.h"
#include "NeoSanctum/Combat/Warning/NSAreaWarningPlaneActor.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSCosmeticHandler_Bombard::GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const
{
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Prepare);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Launch);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Warning);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Impact);
}

void UNSCosmeticHandler_Bombard::HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Prepare)
	{
		HandlePrepareEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Launch)
	{
		HandleLaunchEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Warning)
	{
		HandleWarningEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Bombard_Impact)
	{
		HandleImpactEvent(OwnerActor, EventData);
	}
}

void UNSCosmeticHandler_Bombard::HandlePrepareEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const
{
	if (PrepareSoundID.IsNone())
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
	{
		if (OwnerActor && OwnerActor->GetRootComponent())
		{
			SoundSubsystem->PlaySoundAttached(PrepareSoundID, OwnerActor->GetRootComponent());
		}
		else
		{
			SoundSubsystem->PlaySoundAtLocation(PrepareSoundID, EventData.Location);
		}
	}
}

void UNSCosmeticHandler_Bombard::HandleLaunchEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const
{
	const FRotator Rotation = EventData.Direction.IsNearlyZero()
		                          ? FRotator::ZeroRotator
		                          : EventData.Direction.Rotation();

	if (!LaunchSoundID.IsNone())
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
		{
			SoundSubsystem->PlaySoundAtLocation(LaunchSoundID, EventData.Location);
		}
	}

	if (!LaunchVFXID.IsNone())
	{
		if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor))
		{
			VFXSubsystem->PlayVFXAtLocation(LaunchVFXID, EventData.Location, Rotation, 1.0f);
		}
	}
}

void UNSCosmeticHandler_Bombard::HandleWarningEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const
{
	UWorld* World = GetWorld();
	if (!World || !WarningPlaneClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = EventData.Location + FVector::UpVector * WarningPlaneZOffset;

	ANSAreaWarningPlaneActor* WarningPlane =
		World->SpawnActor<ANSAreaWarningPlaneActor>(
			WarningPlaneClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams);

	if (WarningPlane)
	{
		const float Duration = WarningPlaneDuration > 0.0f
			                       ? WarningPlaneDuration
			                       : FMath::Max(EventData.Duration, 0.01f);

		WarningPlane->InitializeCircleWarning(EventData.Radius, Duration);
	}
}

void UNSCosmeticHandler_Bombard::HandleImpactEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData) const
{
	const float ExplosionScale =
		FMath::Max(EventData.Radius, 1.0f) / FMath::Max(ExplosionBaseRadius, 1.0f);

	if (!ExplosionVFXID.IsNone())
	{
		if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor))
		{
			VFXSubsystem->PlayVFXAtLocation(
				ExplosionVFXID,
				EventData.Location,
				FRotator::ZeroRotator,
				ExplosionScale);
		}
	}

	if (!ImpactSoundID.IsNone())
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
		{
			SoundSubsystem->PlaySoundAtLocation(ImpactSoundID, EventData.Location);
		}
	}
}
