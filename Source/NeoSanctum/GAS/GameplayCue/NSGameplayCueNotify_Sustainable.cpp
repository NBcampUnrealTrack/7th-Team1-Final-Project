// Copyright 2026 One Team. All rights reserved.


#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_Sustainable.h"

#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"

ANSGameplayCueNotify_Sustainable::ANSGameplayCueNotify_Sustainable()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
	bAllowMultipleWhileActiveEvents = false;
}

bool ANSGameplayCueNotify_Sustainable::OnActive_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
)
{
	Super::OnActive_Implementation(MyTarget, Parameters);
	
	PlayAttachedSound(MyTarget, StartSoundID, SoundAttachSocketName);
	SpawnAttachedVFX(MyTarget, StartVFX, VFXAttachSocketName, true);
	
	LoopPresentation(MyTarget);
	
	return true;
}

bool ANSGameplayCueNotify_Sustainable::WhileActive_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
)
{
	Super::WhileActive_Implementation(MyTarget, Parameters);
	
	LoopPresentation(MyTarget);
	
	return true;
}

bool ANSGameplayCueNotify_Sustainable::OnRemove_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
)
{
	Super::OnRemove_Implementation(MyTarget, Parameters);
	
	if (LoopAudioComponent)
	{
		if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(MyTarget))
		{
			SoundSubsystem->StopSound(LoopAudioComponent, LoopSoundFadeOutTime);
		}
		else
		{
			LoopAudioComponent->Stop();
		}
		
		LoopAudioComponent = nullptr;
	}
	
	if (LoopVFXComponent)
	{
		LoopVFXComponent->Deactivate();
		LoopVFXComponent = nullptr;
	}
	
	SpawnAttachedVFX(MyTarget, EndVFX, VFXAttachSocketName, true);
	PlayAttachedSound(MyTarget, EndSoundID, SoundAttachSocketName);
	
	return true;
}

void ANSGameplayCueNotify_Sustainable::LoopPresentation(AActor* MyTarget)
{
	if (!LoopAudioComponent)
	{
		LoopAudioComponent = PlayAttachedSound(MyTarget, LoopSoundID, SoundAttachSocketName);
	}
	
	if (!LoopVFXComponent)
	{
		LoopVFXComponent = SpawnAttachedVFX(MyTarget, LoopVFX, VFXAttachSocketName, false);
	}
}

USceneComponent* ANSGameplayCueNotify_Sustainable::GetAttachComponent(AActor* MyTarget, FName SocketName) const
{
	if (!MyTarget)
	{
		return nullptr;
	}
	
	if (UMeshComponent* MeshComponent = MyTarget->FindComponentByClass<UMeshComponent>())
	{
		if (SocketName.IsNone() || MeshComponent->DoesSocketExist(SocketName))
		{
			return MeshComponent;
		}
	}
	
	return MyTarget->GetRootComponent();
}

UAudioComponent* ANSGameplayCueNotify_Sustainable::PlayAttachedSound(
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

UNiagaraComponent* ANSGameplayCueNotify_Sustainable::SpawnAttachedVFX(
	AActor* MyTarget,
	UNiagaraSystem* NiagaraSystem,
	FName SocketName,
	bool bAutoDestroy
) const
{
	if (!NiagaraSystem)
	{
		return nullptr;
	}
	
	USceneComponent* AttachComponent = GetAttachComponent(MyTarget, SocketName);
	if (!AttachComponent)
	{
		return nullptr;
	}
	
	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComponent,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		bAutoDestroy
	);
}
