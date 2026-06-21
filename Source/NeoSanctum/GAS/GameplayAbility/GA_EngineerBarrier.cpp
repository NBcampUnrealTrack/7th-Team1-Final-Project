// Copyright 2026 One Team. All rights reserved.


#include "GA_EngineerBarrier.h"

#include "GameFramework/Character.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSBarrier.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"

UGA_EngineerBarrier::UGA_EngineerBarrier()
{
}

void UGA_EngineerBarrier::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!BarrierAbilityConfig.BarrierClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float BarrierRadius = 0.0f;
	if (!TryGetBarrierRadius(BarrierRadius))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float BarrierDuration = 0.0f;
	if (!TryGetBarrierDuration(BarrierDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RebuildSetByCallerMagnitudes();
	SpawnBarrierActor(ActorInfo, BarrierRadius, BarrierDuration, SetByCallerMagnitudes);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_EngineerBarrier::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_EngineerBarrier::TryGetBarrierRadius(float& OutBarrierRadius) const
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

bool UGA_EngineerBarrier::TryGetBarrierDuration(float& OutBarrierDuration) const
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

void UGA_EngineerBarrier::RebuildSetByCallerMagnitudes()
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

void UGA_EngineerBarrier::SpawnBarrierActor(
	const FGameplayAbilityActorInfo* ActorInfo,
	float BarrierRadius,
	float BarrierDuration,
	const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes)
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World || !BarrierAbilityConfig.BarrierClass)
	{
		return;
	}

	if (ActiveBarrier)
	{
		ActiveBarrier->Destroy();
		ActiveBarrier = nullptr;
	}

	APawn* OwningPawn = Cast<APawn>(AvatarActor);
	AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;

	ANSBarrier* SpawnedBarrier = World->SpawnActorDeferred<ANSBarrier>(
		BarrierAbilityConfig.BarrierClass,
		AvatarActor->GetActorTransform(),
		AvatarActor,
		OwningPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!SpawnedBarrier)
	{
		return;
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
}
