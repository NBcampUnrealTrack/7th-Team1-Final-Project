// Copyright 2026 One Team. All rights reserved.

#include "ANS_PlayWeaponVFXByID.h"

#include "NiagaraComponent.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"

void UANS_PlayWeaponVFXByID::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp) || VFXID.IsNone())
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FName ResolvedSocketName = NAME_None;
	USceneComponent* AttachComponent = FindWeaponAttachComponent(MeshComp, ResolvedSocketName);
	if (!IsValid(AttachComponent))
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(MeshComp);
	if (!IsValid(VFXSubsystem))
	{
		return;
	}

	StopVFX(MeshComp);

	UNiagaraComponent* NiagaraComponent = VFXSubsystem->SpawnVFXAttached(
		VFXID,
		AttachComponent,
		ResolvedSocketName,
		LocationOffset,
		RotationOffset,
		ScaleMultiplier,
		true);

	if (IsValid(NiagaraComponent))
	{
		ActiveVFXComponents.Add(MeshComp, NiagaraComponent);
	}
}

void UANS_PlayWeaponVFXByID::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	StopVFX(MeshComp);
}

FString UANS_PlayWeaponVFXByID::GetNotifyName_Implementation() const
{
	if (VFXID.IsNone())
	{
		return TEXT("NS Play Weapon VFX By ID");
	}

	return FString::Printf(TEXT("Weapon VFX: %s"), *VFXID.ToString());
}

USceneComponent* UANS_PlayWeaponVFXByID::FindWeaponAttachComponent(
	USkeletalMeshComponent* MeshComp,
	FName& OutSocketName) const
{
	OutSocketName = NAME_None;

	if (!IsValid(MeshComp))
	{
		return nullptr;
	}

	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(MeshComp->GetOwner());
	ANSWeaponBase* CurrentWeapon = PlayerCharacter ? PlayerCharacter->GetCurrentWeapon() : nullptr;
	if (!IsValid(CurrentWeapon))
	{
		return nullptr;
	}

	if (SocketName.IsNone())
	{
		return CurrentWeapon->GetRootComponent();
	}

	TArray<USceneComponent*> SceneComponents;
	CurrentWeapon->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) && SceneComponent->DoesSocketExist(SocketName))
		{
			OutSocketName = SocketName;
			return SceneComponent;
		}
	}

	return bFallbackToWeaponRoot ? CurrentWeapon->GetRootComponent() : nullptr;
}

void UANS_PlayWeaponVFXByID::StopVFX(USkeletalMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent>* FoundComponent = ActiveVFXComponents.Find(MeshComp);
	if (!FoundComponent)
	{
		return;
	}

	if (UNiagaraComponent* NiagaraComponent = FoundComponent->Get())
	{
		// NotifyState 종료 시 Trail 비활성화
		NiagaraComponent->Deactivate();
	}

	ActiveVFXComponents.Remove(MeshComp);
}
