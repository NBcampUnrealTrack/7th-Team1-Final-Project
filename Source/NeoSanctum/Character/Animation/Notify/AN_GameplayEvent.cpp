// Copyright 2026 One Team. All rights reserved.


#include "AN_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

void UAN_GameplayEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}
	
	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld()) return;
	
	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority()) return;
	
	FGameplayEventData EventData;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	
	for (const FGameplayTag& Tag : EventTags)
	{
		if (Tag.IsValid())
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Tag, EventData);
		}
	}
}
