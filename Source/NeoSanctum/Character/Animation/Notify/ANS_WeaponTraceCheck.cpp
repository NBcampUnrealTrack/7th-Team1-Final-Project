// Copyright 2026 One Team. All rights reserved.


#include "ANS_WeaponTraceCheck.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

void UANS_WeaponTraceCheck::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	SendWeaponTraceEvent(MeshComp);
}

void UANS_WeaponTraceCheck::SendWeaponTraceEvent(USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	EventData.OptionalObject = this;

	for (const FGameplayTag& Tag : EventTags)
	{
		if (Tag.IsValid())
		{
			EventData.EventTag = Tag;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Tag, EventData);
		}
	}
}
