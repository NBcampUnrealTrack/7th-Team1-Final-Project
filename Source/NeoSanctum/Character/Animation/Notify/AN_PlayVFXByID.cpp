// Copyright 2026 One Team. All rights reserved.

#include "AN_PlayVFXByID.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSVFXSubsystem.h"

void UAN_PlayVFXByID::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp) || VFXID.IsNone())
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UNSVFXSubsystem* VFXSubsystem = UNSVFXSubsystem::Get(MeshComp);
	if (!IsValid(VFXSubsystem))
	{
		return;
	}

	FName ResolvedSocketName = NAME_None;
	if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		ResolvedSocketName = SocketName;
	}

	switch (VFXMode)
	{
	case ENSAnimNotifyVFXMode::Attached:
		VFXSubsystem->PlayVFXAttached(
			VFXID,
			MeshComp,
			ResolvedSocketName,
			LocationOffset,
			RotationOffset,
			ScaleMultiplier
		);
		break;

	case ENSAnimNotifyVFXMode::AtLocation:
		{
			const FTransform SocketTransform = ResolvedSocketName.IsNone()
				? MeshComp->GetComponentTransform()
				: MeshComp->GetSocketTransform(ResolvedSocketName);
			const FVector VFXLocation = SocketTransform.TransformPosition(LocationOffset);
			const FRotator VFXRotation = (SocketTransform.GetRotation() * RotationOffset.Quaternion()).Rotator();

			VFXSubsystem->PlayVFXAtLocation(
				VFXID,
				VFXLocation,
				VFXRotation,
				ScaleMultiplier
			);
		}
		break;

	default:
		break;
	}
}

FString UAN_PlayVFXByID::GetNotifyName_Implementation() const
{
	if (VFXID.IsNone())
	{
		return TEXT("NS Play VFX By ID");
	}

	return FString::Printf(TEXT("VFX: %s"), *VFXID.ToString());
}
