// Copyright 2026 One Team. All rights reserved.

#include "GA_BarrierBase.h"

#include "GameFramework/Character.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSBarrierBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

bool UGA_BarrierBase::HasValidBarrierConfig() const
{
	return BarrierAbilityConfig.BarrierClass != nullptr;
}

bool UGA_BarrierBase::TryGetBarrierRadius(float& OutBarrierRadius) const
{
	float FinalBarrierRadius = 0.0f;

	if (!TryGetFinalAbilityStat(
		SkillAbilityTag,
		NSGameplayTags::CombatStat_Radius,
		FinalBarrierRadius))
	{
		return false;
	}

	OutBarrierRadius = FMath::Max(FinalBarrierRadius, MinimumBarrierRadius);

	return true;
}

bool UGA_BarrierBase::TryGetBarrierDuration(float& OutBarrierDuration) const
{
	float FinalBarrierDuration = 0.0f;

	if (!TryGetFinalAbilityStat(
		SkillAbilityTag,
		NSGameplayTags::CombatStat_Duration,
		FinalBarrierDuration))
	{
		return false;
	}

	OutBarrierDuration = FMath::Max(FinalBarrierDuration, 0.0f);
	return true;
}

void UGA_BarrierBase::RebuildSetByCallerMagnitudes()
{
	SetByCallerMagnitudes.Reset();

	for (const FNSSetByCallerFromCombatStat& Mapping : BarrierAbilityConfig.SetByCallerMappings)
	{
		if (!Mapping.CombatStatTag.IsValid() || !Mapping.SetByCallerTag.IsValid())
		{
			continue;
		}

		float Magnitude = 0.0f;
		if (!TryGetFinalAbilityStat(
			SkillAbilityTag,
			Mapping.CombatStatTag,
			Magnitude))
		{
			continue;
		}

		FNSSetByCallerMagnitude SetByCallerMagnitude;
		SetByCallerMagnitude.SetByCallerTag = Mapping.SetByCallerTag;
		SetByCallerMagnitude.Magnitude = Magnitude;
		SetByCallerMagnitudes.Add(SetByCallerMagnitude);
	}
}

ANSBarrierBase* UGA_BarrierBase::SpawnBarrierActor(
	const FGameplayAbilityActorInfo* ActorInfo,
	float BarrierRadius,
	float BarrierDuration,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World || !BarrierAbilityConfig.BarrierClass)
	{
		return nullptr;
	}

	if (IsValid(ActiveBarrier))
	{
		ActiveBarrier->Destroy();
		ActiveBarrier = nullptr;
	}

	APawn* OwningPawn = Cast<APawn>(AvatarActor);
	AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;

	ANSBarrierBase* SpawnedBarrier = World->SpawnActorDeferred<ANSBarrierBase>(
		BarrierAbilityConfig.BarrierClass,
		AvatarActor->GetActorTransform(),
		AvatarActor,
		OwningPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!SpawnedBarrier)
	{
		return nullptr;
	}

	USceneComponent* AttachParent = AvatarActor->GetRootComponent();
	if (const ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		if (Character->GetMesh() && !AttachSocketName.IsNone() && Character->GetMesh()->DoesSocketExist(AttachSocketName))
		{
			AttachParent = Character->GetMesh();
		}
	}

	if (AttachParent)
	{
		SpawnedBarrier->AttachToComponent(
			AttachParent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName
		);
		SpawnedBarrier->SetActorRelativeTransform(AttachRelativeTransform);
	}

	SpawnedBarrier->InitializeBarrier(
		OwningPawn,
		OwningController,
		BarrierRadius,
		BarrierDuration,
		BarrierAbilityConfig.InitialAttributeEffectClass,
		InSetByCallerMagnitudes
	);
	SpawnedBarrier->FinishSpawning(SpawnedBarrier->GetActorTransform());
	SpawnedBarrier->ForceNetUpdate();
	ActiveBarrier = SpawnedBarrier;

	return SpawnedBarrier;
}
