// Copyright 2026 One Team. All rights reserved.


#include "NSCosmeticHandler_TitanWalkerDeath.h"

#include "Components/SkeletalMeshComponent.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Enemy.h"
#include "TimerManager.h"

void UNSCosmeticHandler_TitanWalkerDeath::GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const
{
	OutEventTags.Add(NSGameplayTags::Cosmetic_Enemy_TitanWalker_Death_Explosion);
}

void UNSCosmeticHandler_TitanWalkerDeath::HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData)
{
	if (!IsValid(OwnerActor) || EventData.EventTag != NSGameplayTags::Cosmetic_Enemy_TitanWalker_Death_Explosion)
	{
		return;
	}

	ClearActiveTimers();

	TWeakObjectPtr<AActor> WeakOwnerActor = OwnerActor;

	for (const FNSTitanWalkerDeathExplosionStep& Step : ExplosionSteps)
	{
		if (Step.Delay <= 0.0f)
		{
			PlayExplosionStep(WeakOwnerActor, Step);
			continue;
		}

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, WeakOwnerActor, Step]()
			{
				PlayExplosionStep(WeakOwnerActor, Step);
			}),
			Step.Delay,
			false);

		ActiveTimerHandles.Add(TimerHandle);
	}
}

void UNSCosmeticHandler_TitanWalkerDeath::PlayExplosionStep(
	TWeakObjectPtr<AActor> WeakOwnerActor,
	FNSTitanWalkerDeathExplosionStep Step) const
{
	AActor* OwnerActor = WeakOwnerActor.Get();
	if (!IsValid(OwnerActor) || ExplosionVFXID.IsNone())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = ResolveEnemyMesh(OwnerActor);
	if (!MeshComponent)
	{
		return;
	}

	FVector SpawnLocation = MeshComponent->GetComponentLocation();

	if (!Step.BoneName.IsNone() && MeshComponent->GetBoneIndex(Step.BoneName) != INDEX_NONE)
	{
		SpawnLocation = MeshComponent->GetBoneLocation(Step.BoneName, EBoneSpaces::WorldSpace);
	}

	SpawnLocation += Step.LocationOffset;

	if (UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(OwnerActor))
	{
		VFXSubsystem->PlayVFXAtLocation(
			ExplosionVFXID,
			SpawnLocation,
			FRotator::ZeroRotator,
			GlobalScaleMultiplier * Step.ScaleMultiplier);
	}
}

USkeletalMeshComponent* UNSCosmeticHandler_TitanWalkerDeath::ResolveEnemyMesh(AActor* OwnerActor) const
{
	if (INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(OwnerActor))
	{
		if (USkeletalMeshComponent* EnemyMesh = EnemyAgent->GetEnemyMesh())
		{
			return EnemyMesh;
		}
	}

	return OwnerActor ? OwnerActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

void UNSCosmeticHandler_TitanWalkerDeath::ClearActiveTimers()
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& TimerHandle : ActiveTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	ActiveTimerHandles.Reset();
}

void UNSCosmeticHandler_TitanWalkerDeath::BeginDestroy()
{
	ClearActiveTimers();
	Super::BeginDestroy();
}
