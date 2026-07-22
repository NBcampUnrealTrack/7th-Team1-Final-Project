// Copyright 2026 One Team. All rights reserved.

#include "NSCosmeticHandler_Laser.h"

#include "NiagaraComponent.h"
#include "NSEnemyCosmeticComponent.h"
#include "Components/AudioComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSCosmeticHandler_Laser::GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const
{
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeStart);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeUpdate);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamStart);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamUpdate);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_Stop);
}

void UNSCosmeticHandler_Laser::HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeStart)
	{
		HandleChargeStartEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_ChargeUpdate)
	{
		HandleChargeUpdateEvent(EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamStart)
	{
		HandleBeamStartEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_BeamUpdate)
	{
		HandleBeamUpdateEvent(EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Laser_Stop)
	{
		HandleStopEvent(EventData);
	}
}

void UNSCosmeticHandler_Laser::HandleChargeStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	StopLaserCosmetic(EventData.InstanceId);

	FNSActiveLaserCosmetic& ActiveLaser = ActiveLasers.FindOrAdd(EventData.InstanceId);

	if (!LaserChargeSoundID.IsNone() && OwnerActor && OwnerActor->GetRootComponent())
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
		{
			ActiveLaser.ChargeAudioComponent =
				SoundSubsystem->PlaySoundAttached(LaserChargeSoundID, OwnerActor->GetRootComponent());
		}
	}

	if (!LaserChargeVFXID.IsNone())
	{
		if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor))
		{
			
			const FRotator Rotation = EventData.Direction.IsNearlyZero()
				                          ? FRotator::ZeroRotator
				                          : EventData.Direction.Rotation();

			ActiveLaser.ChargeVFXComponent =
				VFXSubsystem->SpawnVFXAtLocation(
					LaserChargeVFXID,
					EventData.Location,
					Rotation,
					1.0f,
					false);

			if (IsValid(ActiveLaser.ChargeVFXComponent))
			{
				if (!LaserChargeDurationParameterName.IsNone())
				{
					ActiveLaser.ChargeVFXComponent->SetVariableFloat(
						LaserChargeDurationParameterName,
						FMath::Max(EventData.Duration, 0.0f));
				}

				ActiveLaser.ChargeVFXComponent->Activate(true);
			}
		}
	}

	UpdateChargeVFX(ActiveLaser, EventData);
}

void UNSCosmeticHandler_Laser::HandleChargeUpdateEvent(const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveLaserCosmetic* ActiveLaser = ActiveLasers.Find(EventData.InstanceId);
	if (!ActiveLaser)
	{
		return;
	}

	UpdateChargeVFX(*ActiveLaser, EventData);
}

void UNSCosmeticHandler_Laser::HandleBeamStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveLaserCosmetic& ActiveLaser = ActiveLasers.FindOrAdd(EventData.InstanceId);

	StopChargeVFX(ActiveLaser);

	if (!bLaserBeamSoundIncludedInChargeSound)
	{
		StopChargeSound(ActiveLaser);

		if (!LaserBeamSoundID.IsNone() && OwnerActor && OwnerActor->GetRootComponent())
		{
			if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
			{
				ActiveLaser.BeamAudioComponent =
					SoundSubsystem->PlaySoundAttached(LaserBeamSoundID, OwnerActor->GetRootComponent());
			}
		}
	}

	UpdateBeamVFX(ActiveLaser, OwnerActor, EventData);
}

void UNSCosmeticHandler_Laser::HandleBeamUpdateEvent(const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveLaserCosmetic* ActiveLaser = ActiveLasers.Find(EventData.InstanceId);
	if (!ActiveLaser)
	{
		return;
	}

	AActor* OwnerActor = OwnerComponent ? OwnerComponent->GetOwner() : nullptr;
	UpdateBeamVFX(*ActiveLaser, OwnerActor, EventData);
}

void UNSCosmeticHandler_Laser::HandleStopEvent(const FNSCosmeticEventNetData& EventData)
{
	StopLaserCosmetic(EventData.InstanceId);
}

void UNSCosmeticHandler_Laser::UpdateChargeVFX(
	FNSActiveLaserCosmetic& ActiveLaser,
	const FNSCosmeticEventNetData& EventData) const
{
	if (!IsValid(ActiveLaser.ChargeVFXComponent))
	{
		return;
	}

	const FRotator Rotation = EventData.Direction.IsNearlyZero()
		                          ? FRotator::ZeroRotator
		                          : EventData.Direction.Rotation();

	ActiveLaser.ChargeVFXComponent->SetWorldLocationAndRotation(EventData.Location, Rotation);

	if (!LaserChargeDurationParameterName.IsNone())
	{
		ActiveLaser.ChargeVFXComponent->SetVariableFloat(
			LaserChargeDurationParameterName,
			FMath::Max(EventData.Duration, 0.0f));
	}
}

void UNSCosmeticHandler_Laser::UpdateBeamVFX(
	FNSActiveLaserCosmetic& ActiveLaser,
	AActor* OwnerActor,
	const FNSCosmeticEventNetData& EventData) const
{
	if (LaserBeamVFXID.IsNone())
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor);
	if (!VFXSubsystem)
	{
		return;
	}

	TArray<FNSCosmeticEventPointNetData> BeamPoints = EventData.Points;
	if (BeamPoints.IsEmpty())
	{
		FNSCosmeticEventPointNetData PointData;
		PointData.Location = EventData.Location;
		PointData.EndLocation = EventData.EndLocation;
		PointData.Direction = EventData.Direction;
		BeamPoints.Add(PointData);
	}

	while (ActiveLaser.BeamVFXComponents.Num() < BeamPoints.Num())
	{
		ActiveLaser.BeamVFXComponents.Add(nullptr);
	}

	const float BeamVisualWidth = GetLaserBeamVisualWidth(EventData);

	for (int32 Index = 0; Index < BeamPoints.Num(); ++Index)
	{
		const FNSCosmeticEventPointNetData& PointData = BeamPoints[Index];

		FVector Direction = PointData.Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = (PointData.EndLocation - PointData.Location).GetSafeNormal();
		}

		const float BeamLength = FVector::Dist(PointData.Location, PointData.EndLocation);
		if (Direction.IsNearlyZero() || BeamLength <= KINDA_SMALL_NUMBER)
		{
			DestroyLaserVFXComponent(ActiveLaser.BeamVFXComponents[Index].Get());
			ActiveLaser.BeamVFXComponents[Index] = nullptr;
			continue;
		}

		const FRotator BeamRotation = GetLaserBeamVFXRotation(Direction);
		UNiagaraComponent* VFX = ActiveLaser.BeamVFXComponents[Index];

		if (!IsValid(VFX))
		{
			VFX = VFXSubsystem->SpawnVFXAtLocation(
				LaserBeamVFXID,
				PointData.Location,
				BeamRotation,
				LaserBeamVFXScale,
				false);

			ActiveLaser.BeamVFXComponents[Index] = VFX;
		}

		if (IsValid(VFX))
		{
			VFX->SetWorldLocationAndRotation(PointData.Location, BeamRotation);
			VFX->SetWorldScale3D(FVector::OneVector * LaserBeamVFXScale);
			ApplyLaserBeamVFXParameters(VFX, PointData, BeamVisualWidth);

			if (!VFX->IsActive())
			{
				VFX->Activate(true);
			}
		}
	}

	for (int32 Index = BeamPoints.Num(); Index < ActiveLaser.BeamVFXComponents.Num(); ++Index)
	{
		DestroyLaserVFXComponent(ActiveLaser.BeamVFXComponents[Index].Get());
		ActiveLaser.BeamVFXComponents[Index] = nullptr;
	}

	ActiveLaser.BeamVFXComponents.SetNum(BeamPoints.Num());
}

void UNSCosmeticHandler_Laser::DestroyLaserVFXComponent(UNiagaraComponent* VFX) const
{
	if (!IsValid(VFX))
	{
		return;
	}

	VFX->DeactivateImmediate();
	VFX->DestroyComponent();
}

void UNSCosmeticHandler_Laser::StopChargeVFX(FNSActiveLaserCosmetic& ActiveLaser) const
{
	DestroyLaserVFXComponent(ActiveLaser.ChargeVFXComponent.Get());
	ActiveLaser.ChargeVFXComponent = nullptr;
}

void UNSCosmeticHandler_Laser::StopChargeSound(FNSActiveLaserCosmetic& ActiveLaser) const
{
	if (!ActiveLaser.ChargeAudioComponent)
	{
		return;
	}

	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerComponent))
	{
		SoundSubsystem->StopSound(ActiveLaser.ChargeAudioComponent, SoundFadeOutTime);
	}
	else
	{
		ActiveLaser.ChargeAudioComponent->Stop();
	}

	ActiveLaser.ChargeAudioComponent = nullptr;
}

void UNSCosmeticHandler_Laser::StopLaserCosmetic(int32 InstanceId)
{
	if (InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveLaserCosmetic* ActiveLaser = ActiveLasers.Find(InstanceId);
	if (!ActiveLaser)
	{
		return;
	}

	StopChargeVFX(*ActiveLaser);

	for (TObjectPtr<UNiagaraComponent>& VFX : ActiveLaser->BeamVFXComponents)
	{
		DestroyLaserVFXComponent(VFX.Get());
		VFX = nullptr;
	}

	ActiveLaser->BeamVFXComponents.Reset();
	StopChargeSound(*ActiveLaser);

	if (ActiveLaser->BeamAudioComponent)
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerComponent))
		{
			SoundSubsystem->StopSound(ActiveLaser->BeamAudioComponent, SoundFadeOutTime);
		}
		else
		{
			ActiveLaser->BeamAudioComponent->Stop();
		}

		ActiveLaser->BeamAudioComponent = nullptr;
	}

	ActiveLasers.Remove(InstanceId);
}

float UNSCosmeticHandler_Laser::GetLaserBeamVisualWidth(const FNSCosmeticEventNetData& EventData) const
{
	if (bOverrideLaserBeamVisualWidth)
	{
		return FMath::Max(LaserBeamVisualWidthOverride, 0.0f);
	}

	const float Width = FMath::Max(EventData.Radius, 0.0f) * LaserBeamWidthRadiusMultiplier;
	return FMath::Max(Width, LaserBeamMinVisualWidth);
}

FRotator UNSCosmeticHandler_Laser::GetLaserBeamVFXRotation(const FVector& Direction) const
{
	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return FRotator::ZeroRotator;
	}

	return FRotationMatrix::MakeFromY(SafeDirection).Rotator();
}

void UNSCosmeticHandler_Laser::BeginDestroy()
{
	TArray<int32> InstanceIds;
	ActiveLasers.GetKeys(InstanceIds);

	for (int32 InstanceId : InstanceIds)
	{
		StopLaserCosmetic(InstanceId);
	}

	Super::BeginDestroy();
}

void UNSCosmeticHandler_Laser::ApplyLaserBeamVFXParameters(
	UNiagaraComponent* VFX,
	const FNSCosmeticEventPointNetData& PointData,
	float BeamVisualWidth) const
{
	if (!IsValid(VFX))
	{
		return;
	}

	const float BeamLength = FVector::Dist(PointData.Location, PointData.EndLocation);

	if (!LaserBeamLengthParameterName.IsNone())
	{
		VFX->SetVariableFloat(LaserBeamLengthParameterName, BeamLength);
	}

	if (!LaserBeamWidthParameterName.IsNone())
	{
		VFX->SetVariableFloat(LaserBeamWidthParameterName, BeamVisualWidth);
	}
}
