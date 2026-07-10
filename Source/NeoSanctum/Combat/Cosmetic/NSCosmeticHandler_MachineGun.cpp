// Copyright 2026 One Team. All rights reserved.


#include "NSCosmeticHandler_MachineGun.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSCosmeticHandler_MachineGun::GetHandledEventTags(
	TArray<FGameplayTag>& OutEventTags) const
{
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_MachineGun_Fire);
}

void UNSCosmeticHandler_MachineGun::HandleEvent(
	AActor* OwnerActor,
	const FNSCosmeticEventNetData& EventData)
{
	if (EventData.EventTag != NSGameplayTags::Cosmetic_Enemy_TitanWalker_MachineGun_Fire)
	{
		return;
	}

	if (!FireSoundID.IsNone())
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
		{
			SoundSubsystem->PlaySoundAtLocation(FireSoundID, EventData.Location);
		}
	}

	if (!FireVFXID.IsNone())
	{
		if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor))
		{
			const FRotator VFXRotation = EventData.Direction.IsNearlyZero()
				                             ? FRotator::ZeroRotator
				                             : EventData.Direction.Rotation();

			VFXSubsystem->PlayVFXAtLocation(
				FireVFXID,
				EventData.Location,
				VFXRotation,
				FireVFXScaleMultiplier);
		}
	}
}
