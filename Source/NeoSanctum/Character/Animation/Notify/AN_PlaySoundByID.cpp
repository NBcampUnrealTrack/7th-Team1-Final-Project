// Copyright 2026 One Team. All rights reserved.


#include "AN_PlaySoundByID.h"

#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"

void UAN_PlaySoundByID::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp) || SoundID.IsNone())
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(MeshComp);

	if (!IsValid(SoundSubsystem))
	{
		return;
	}

	FName ResolvedSocketName = NAME_None;

	if (!SocketName.IsNone())
	{
		if (MeshComp->DoesSocketExist(SocketName))
		{
			ResolvedSocketName = SocketName;
		}
	}

	switch (SoundMode)
	{
	case ENSAnimNotifySoundMode::Attached:
		SoundSubsystem->PlaySoundAttached(
			SoundID,
			MeshComp,
			ResolvedSocketName,
			PitchMultiplier
		);
		break;

	case ENSAnimNotifySoundMode::AtLocation:
		{
			const FVector SoundLocation = ResolvedSocketName.IsNone()
				                              ? MeshComp->GetComponentLocation()
				                              : MeshComp->GetSocketLocation(ResolvedSocketName);

			SoundSubsystem->PlaySoundAtLocation(
				SoundID,
				SoundLocation,
				PitchMultiplier
			);
		}
		break;

	default:
		break;
	}
}

FString UAN_PlaySoundByID::GetNotifyName_Implementation() const
{
	if (SoundID.IsNone())
	{
		return TEXT("NS Play Sound By ID");
	}

	return FString::Printf(TEXT("Sound: %s"), *SoundID.ToString());
}
