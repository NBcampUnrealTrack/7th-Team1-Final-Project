// Copyright 2026 One Team. All rights reserved.


#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_Sustainable.h"

#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
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
	
	PlaySound(MyTarget, Parameters, StartSoundID, SoundSpawnMode, SoundAttachComponentName, SoundAttachSocketName);
	SpawnVFX(MyTarget, Parameters, StartVFX, VFXSpawnMode, VFXAttachComponentName, VFXAttachSocketName, true);
	
	LoopPresentation(MyTarget, Parameters);
	
	return true;
}

bool ANSGameplayCueNotify_Sustainable::WhileActive_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
)
{
	Super::WhileActive_Implementation(MyTarget, Parameters);
	
	LoopPresentation(MyTarget, Parameters);
	
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
	
	SpawnVFX(MyTarget, Parameters, EndVFX, VFXSpawnMode, VFXAttachComponentName, VFXAttachSocketName, true);
	PlaySound(MyTarget, Parameters, EndSoundID, SoundSpawnMode, SoundAttachComponentName, SoundAttachSocketName);
	
	return true;
}

void ANSGameplayCueNotify_Sustainable::LoopPresentation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!LoopAudioComponent)
	{
		LoopAudioComponent = PlaySound(
			MyTarget,
			Parameters,
			LoopSoundID,
			SoundSpawnMode,
			SoundAttachComponentName,
			SoundAttachSocketName
		);
	}
	
	if (!LoopVFXComponent)
	{
		LoopVFXComponent = SpawnVFX(
			MyTarget,
			Parameters,
			LoopVFX,
			VFXSpawnMode,
			VFXAttachComponentName,
			VFXAttachSocketName,
			false
		);
	}
}

USceneComponent* ANSGameplayCueNotify_Sustainable::GetAttachComponent(
	AActor* MyTarget,
	FName ComponentName,
	FName SocketName,
	FName& OutSocketName
) const
{
	OutSocketName = NAME_None;
	
	if (!MyTarget)
	{
		return nullptr;
	}
	
	// MeshComponent 이름을 따로 설정한 경우
	if (!ComponentName.IsNone())
	{
		TArray<USceneComponent*> SceneComponents;
		MyTarget->GetComponents<USceneComponent>(SceneComponents);
		
		// 원하는 소켓 이름이 있는지 SceneComponent 아래의 메쉬를 모두 탐색
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (!IsValid(SceneComponent) || SceneComponent->GetFName() != ComponentName)
			{
				continue;
			}
			
			if (!SocketName.IsNone() && !SceneComponent->DoesSocketExist(SocketName))
			{
				return nullptr;
			}
			
			OutSocketName = SocketName;
			return SceneComponent;
		}
		
		return nullptr;
	}
	
	TArray<UMeshComponent*> MeshComponents;
	MyTarget->GetComponents<UMeshComponent>(MeshComponents);
	
	// 메쉬는 설정 안했는데, 소켓 이름만 따로 설정한 경우
	if (!SocketName.IsNone())
	{
		// 메쉬 컴포넌트를 모두 탐색해서 소켓 이름이 존재하는지 탐색
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (IsValid(MeshComponent) && MeshComponent->DoesSocketExist(SocketName))
			{
				OutSocketName = SocketName;
				return MeshComponent;
			}
		}
		
		return nullptr;
	}
	
	// 둘 다 설정 안한 경우지만 메쉬 컴포넌트가 있는 경우
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			return MeshComponent;
		}
	}
	
	return MyTarget->GetRootComponent();
}

UAudioComponent* ANSGameplayCueNotify_Sustainable::PlaySound(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters,
	FName SoundID,
	ENSGameplayCueSpawnMode SpawnMode,
	FName ComponentName,
	FName SocketName
) const
{
	if (SoundID.IsNone())
	{
		return nullptr;
	}
	
	UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(MyTarget);
	if (!SoundSubsystem)
	{
		return nullptr;
	}
	
	if (SpawnMode == ENSGameplayCueSpawnMode::Attached)
	{
		FName ResolvedSocketName;
		USceneComponent* AttachComponent = GetAttachComponent(
			MyTarget,
			ComponentName,
			SocketName,
			ResolvedSocketName
		);
		
		if (AttachComponent)
		{
			return SoundSubsystem->PlaySoundAttached(SoundID, AttachComponent, ResolvedSocketName);
		}

		return nullptr;
	}

	return SoundSubsystem->PlaySoundAtLocation(SoundID, Parameters.Location);
}

UNiagaraComponent* ANSGameplayCueNotify_Sustainable::SpawnVFX(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters,
	UNiagaraSystem* NiagaraSystem,
	ENSGameplayCueSpawnMode SpawnMode,
	FName ComponentName,
	FName SocketName,
	bool bAutoDestroy
) const
{
	if (!NiagaraSystem)
	{
		return nullptr;
	}
	
	if (SpawnMode == ENSGameplayCueSpawnMode::Attached)
	{
		FName ResolvedSocketName;
		USceneComponent* AttachComponent = GetAttachComponent(
			MyTarget,
			ComponentName,
			SocketName,
			ResolvedSocketName
		);
		
		if (!AttachComponent)
		{
			return nullptr;
		}

		return UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			AttachComponent,
			ResolvedSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			bAutoDestroy
		);
	}
	
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MyTarget,
		NiagaraSystem,
		Parameters.Location,
		Parameters.Normal.Rotation(),
		FVector::OneVector,
		bAutoDestroy
	);
}
