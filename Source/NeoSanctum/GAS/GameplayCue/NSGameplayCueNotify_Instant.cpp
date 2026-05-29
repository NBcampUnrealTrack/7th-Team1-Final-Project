// Copyright 2026 One Team. All rights reserved.


#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_Instant.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"

bool UNSGameplayCueNotify_Instant::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);
	
	PlayAttachedSound(MyTarget, ExecuteSoundID, SoundAttachSocketName);
	SpawnAttachedVFX(MyTarget, ExecuteVFX, VFXAttachSocketName);
	
	return true;
}

USceneComponent* UNSGameplayCueNotify_Instant::GetAttachComponent(AActor* MyTarget, FName SocketName) const
{
	if (!MyTarget)
	{
		return nullptr;
	}
	
	if (USkeletalMeshComponent* MeshComponent = MyTarget->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (SocketName.IsNone() || MeshComponent->DoesSocketExist(SocketName))
		{
			return MeshComponent;
		}
	}
	
	return MyTarget->GetRootComponent();
}

UAudioComponent* UNSGameplayCueNotify_Instant::PlayAttachedSound(
	AActor* MyTarget,
	FName SoundID,
	FName SocketName
) const
{
	if (SoundID.IsNone())
	{
		return nullptr;
	}
	
	UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(MyTarget);
	USceneComponent* AttachComponent = GetAttachComponent(MyTarget, SocketName);
	if (!SoundSubsystem || !AttachComponent)
	{
		return nullptr;
	}
	
	return SoundSubsystem->PlaySoundAttached(SoundID, AttachComponent, SocketName);
}

void UNSGameplayCueNotify_Instant::SpawnAttachedVFX(
	AActor* MyTarget,
	UNiagaraSystem* NiagaraSystem,
	FName SocketName
) const
{
	if (!NiagaraSystem)
	{
		return;
	}
	
	USceneComponent* AttachComponent = GetAttachComponent(MyTarget, SocketName);
	if (!AttachComponent)
	{
		return;
	}
	
	UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComponent,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		true
	);
}
