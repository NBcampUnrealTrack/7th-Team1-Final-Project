// Copyright 2026 One Team. All rights reserved.


#include "AN_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

void UAN_GameplayEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner() && EventTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			MeshComp->GetOwner(), EventTag, FGameplayEventData());
	}
}
