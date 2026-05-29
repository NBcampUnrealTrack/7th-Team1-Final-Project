// Copyright 2026 One Team. All rights reserved.


#include "AN_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

void UAN_GameplayEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	
	FGameplayEventData EventData;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	
	for (const FGameplayTag& Tag : EventTags)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Tag, EventData);
	}
}
