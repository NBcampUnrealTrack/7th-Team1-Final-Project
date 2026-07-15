// Copyright 2026 One Team. All rights reserved.


#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_Instant.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"

bool UNSGameplayCueNotify_Instant::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);
	
	PlaySound(MyTarget, Parameters, ExecuteSoundID, SoundSpawnMode, SoundAttachComponentName, SoundAttachSocketName);
	SpawnVFX(MyTarget, Parameters, ExecuteVFXID, VFXSpawnMode, VFXAttachComponentName, VFXAttachSocketName);
	
	return true;
}

USceneComponent* UNSGameplayCueNotify_Instant::GetAttachComponent(
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

UAudioComponent* UNSGameplayCueNotify_Instant::PlaySound(
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

void UNSGameplayCueNotify_Instant::SpawnVFX(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters,
	FName VFXID,
	ENSGameplayCueSpawnMode SpawnMode,
	FName ComponentName,
	FName SocketName
) const
{
	if (VFXID.IsNone())
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(MyTarget);
	if (!VFXSubsystem)
	{
		return;
	}

	// 실제 폭발 반경과 기본 반경의 비율을 기존 VFX 크기에 반영해줌.
	float FinalVFXScaleMultiplier = VFXScaleMultiplier;

	if (bScaleVFXByEffectRadius && VFXBaseRadius > KINDA_SMALL_NUMBER && Parameters.RawMagnitude > 0.0f)
	{
		// 기본 반경과 실제 반경의 비율만 기존 이펙트 크기에 곱해줌.
		const float RadiusScale = Parameters.RawMagnitude / VFXBaseRadius;
		FinalVFXScaleMultiplier *= RadiusScale;
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
			VFXSubsystem->PlayVFXAttached(
				VFXID,
				AttachComponent,
				ResolvedSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				FinalVFXScaleMultiplier);
			return;
		}
		
		return;
	}
	
	VFXSubsystem->PlayVFXAtLocation(
		VFXID,
		Parameters.Location,
		Parameters.Normal.Rotation(),
		FinalVFXScaleMultiplier);
}
