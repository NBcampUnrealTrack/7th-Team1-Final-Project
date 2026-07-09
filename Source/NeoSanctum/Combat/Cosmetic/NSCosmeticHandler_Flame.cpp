// Copyright 2026 One Team. All rights reserved.


#include "NSCosmeticHandler_Flame.h"

#include "NiagaraComponent.h"
#include "NSEnemyCosmeticComponent.h"
#include "Components/AudioComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"

void UNSCosmeticHandler_Flame::GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const
{
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Start);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Update);
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Stop);
}

void UNSCosmeticHandler_Flame::HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Start)
	{
		HandleStartEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Update)
	{
		HandleUpdateEvent(OwnerActor, EventData);
	}
	else if (EventData.EventTag == NSGameplayTags::Cosmetic_Enemy_TitanWalker_Flame_Stop)
	{
		HandleStopEvent(EventData);
	}
}

void UNSCosmeticHandler_Flame::HandleStartEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	StopFlameCosmetic(EventData.InstanceId);

	FNSActiveFlameCosmetic& ActiveFlame = ActiveFlames.FindOrAdd(EventData.InstanceId);

	if (!FlameSoundID.IsNone() && OwnerActor && OwnerActor->GetRootComponent())
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
		{
			ActiveFlame.AudioComponent =
				SoundSubsystem->PlaySoundAttached(FlameSoundID, OwnerActor->GetRootComponent());
		}
	}

	UpdateFlameVFX(ActiveFlame, OwnerActor, EventData);
}

void UNSCosmeticHandler_Flame::HandleUpdateEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (EventData.InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveFlameCosmetic* ActiveFlame = ActiveFlames.Find(EventData.InstanceId);
	if (!ActiveFlame)
	{
		return;
	}

	UpdateFlameVFX(*ActiveFlame, OwnerActor, EventData);
}

void UNSCosmeticHandler_Flame::HandleStopEvent(const FNSCosmeticEventNetData& EventData)
{
	StopFlameCosmetic(EventData.InstanceId);
}

void UNSCosmeticHandler_Flame::UpdateFlameVFX(
	FNSActiveFlameCosmetic& ActiveFlame,
	AActor* OwnerActor,
	const FNSCosmeticEventNetData& EventData) const
{
	if (FlameVFXID.IsNone())
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor);
	if (!VFXSubsystem)
	{
		return;
	}

	TArray<FTransform> DesiredTransforms;
	BuildFlameVFXTransforms(EventData, DesiredTransforms);

	while (ActiveFlame.VFXComponents.Num() < DesiredTransforms.Num())
	{
		ActiveFlame.VFXComponents.Add(nullptr);
	}

	for (int32 Index = 0; Index < DesiredTransforms.Num(); ++Index)
	{
		const FTransform& Transform = DesiredTransforms[Index];
		UNiagaraComponent* VFX = ActiveFlame.VFXComponents[Index];

		if (!IsValid(VFX))
		{
			VFX = VFXSubsystem->SpawnVFXAtLocation(
				FlameVFXID,
				Transform.GetLocation(),
				Transform.Rotator(),
				FlameVFXComponentScale,
				false);

			ActiveFlame.VFXComponents[Index] = VFX;

			if (IsValid(VFX))
			{
				if (!FlameScaleParameterName.IsNone())
				{
					VFX->SetVariableFloat(FlameScaleParameterName, NiagaraFlameScale);
				}

				if (!SpawnRateParameterName.IsNone())
				{
					VFX->SetVariableFloat(SpawnRateParameterName, NiagaraSpawnRate);
				}

				VFX->Activate(true);
			}
		}

		if (IsValid(VFX))
		{
			VFX->SetWorldLocationAndRotation(Transform.GetLocation(), Transform.Rotator());
			VFX->SetWorldScale3D(FVector::OneVector * FlameVFXComponentScale);
		}
	}

	for (int32 Index = DesiredTransforms.Num(); Index < ActiveFlame.VFXComponents.Num(); ++Index)
	{
		DestroyFlameVFXComponent(ActiveFlame.VFXComponents[Index].Get());
		ActiveFlame.VFXComponents[Index] = nullptr;
	}

	ActiveFlame.VFXComponents.SetNum(DesiredTransforms.Num());
}

void UNSCosmeticHandler_Flame::DestroyFlameVFXComponent(UNiagaraComponent* VFX) const
{
	if (!IsValid(VFX))
	{
		return;
	}

	VFX->DeactivateImmediate();
	VFX->DestroyComponent();
}

void UNSCosmeticHandler_Flame::BuildFlameVFXTransforms(
	const FNSCosmeticEventNetData& EventData,
	TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	const float EffectiveRange = FMath::Max(EventData.Range, 0.0f);
	if (EffectiveRange <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float ConeHalfAngleRadians =
		FMath::DegreesToRadians(FMath::Max(EventData.ConeHalfAngle, 0.0f));

	const float ConeTan = FMath::Tan(ConeHalfAngleRadians);
	const float BaseRadius = FMath::Max(EventData.Radius, 0.0f);

	for (const FNSCosmeticEventPointNetData& Point : EventData.Points)
	{
		const FVector Direction = Point.Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			continue;
		}

		const FRotator SpawnRotation = Direction.Rotation() + RotationOffset;
		const FVector RightVector =
			FRotationMatrix(Direction.Rotation()).GetUnitAxis(EAxis::Y);

		TArray<FTransform> PointTransforms;

		const float FirstDistance = FMath::Min(StartOffset, EffectiveRange);
		const float SafeForwardSpacing = FMath::Max(ForwardSpacing, 1.0f);
		const float SafeLateralSpacing = FMath::Max(LateralSpacing, 1.0f);

		TArray<float> SampleDistances;
		SampleDistances.Add(FirstDistance);

		for (float Distance = FirstDistance + SafeForwardSpacing;
		     Distance < EffectiveRange;
		     Distance += SafeForwardSpacing)
		{
			SampleDistances.Add(Distance);
		}

		if (EffectiveRange > FirstDistance + KINDA_SMALL_NUMBER)
		{
			SampleDistances.Add(EffectiveRange);
		}

		for (const float Distance : SampleDistances)
		{
			const bool bSocketPoint = Distance <= FirstDistance + KINDA_SMALL_NUMBER;
			const float HalfWidth = bSocketPoint
				                        ? 0.0f
				                        : BaseRadius + ConeTan * Distance;

			const int32 LateralCount = HalfWidth <= KINDA_SMALL_NUMBER
				                           ? 1
				                           : FMath::Clamp(
					                           FMath::FloorToInt((HalfWidth * 2.0f) / SafeLateralSpacing) + 1,
					                           1,
					                           3);

			for (int32 Index = 0; Index < LateralCount; ++Index)
			{
				const float Alpha = LateralCount == 1
					                    ? 0.5f
					                    : static_cast<float>(Index) / static_cast<float>(LateralCount - 1);

				const float LateralOffset = FMath::Lerp(-HalfWidth, HalfWidth, Alpha);

				const FVector SpawnLocation =
					Point.Location +
					Direction * Distance +
					RightVector * LateralOffset;

				PointTransforms.Add(FTransform(SpawnRotation, SpawnLocation));
			}
		}

		if (MaxVFXPerEmitter > 0 && PointTransforms.Num() > MaxVFXPerEmitter)
		{
			TArray<FTransform> SampledTransforms;

			if (MaxVFXPerEmitter == 1)
			{
				SampledTransforms.Add(PointTransforms[0]);
			}
			else
			{
				const float Step =
					static_cast<float>(PointTransforms.Num() - 1) /
					static_cast<float>(MaxVFXPerEmitter - 1);

				for (int32 Index = 0; Index < MaxVFXPerEmitter; ++Index)
				{
					const int32 SourceIndex =
						FMath::Clamp(
							FMath::RoundToInt(Index * Step),
							0,
							PointTransforms.Num() - 1);

					SampledTransforms.Add(PointTransforms[SourceIndex]);
				}
			}

			PointTransforms = MoveTemp(SampledTransforms);
		}

		OutTransforms.Append(PointTransforms);
	}
}

void UNSCosmeticHandler_Flame::StopFlameCosmetic(int32 InstanceId)
{
	if (InstanceId == INDEX_NONE)
	{
		return;
	}

	FNSActiveFlameCosmetic* ActiveFlame = ActiveFlames.Find(InstanceId);
	if (!ActiveFlame)
	{
		return;
	}

	for (TObjectPtr<UNiagaraComponent>& VFX : ActiveFlame->VFXComponents)
	{
		DestroyFlameVFXComponent(VFX.Get());
		VFX = nullptr;
	}

	ActiveFlame->VFXComponents.Reset();

	if (ActiveFlame->AudioComponent)
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerComponent))
		{
			SoundSubsystem->StopSound(ActiveFlame->AudioComponent, SoundFadeOutTime);
		}
		else
		{
			ActiveFlame->AudioComponent->Stop();
		}

		ActiveFlame->AudioComponent = nullptr;
	}

	ActiveFlames.Remove(InstanceId);
}

void UNSCosmeticHandler_Flame::BeginDestroy()
{
	TArray<int32> InstanceIds;
	ActiveFlames.GetKeys(InstanceIds);

	for (int32 InstanceId : InstanceIds)
	{
		StopFlameCosmetic(InstanceId);
	}

	Super::BeginDestroy();
}
