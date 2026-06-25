// Copyright 2026 One Team. All rights reserved.


#include "AN_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"

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
	if (!IsValid(Owner)) return;

	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bShouldSendEvent =
		(bSendOnAuthority && Owner->HasAuthority()) ||
		(bSendOnLocallyControlled && OwnerPawn && OwnerPawn->IsLocallyControlled());

	if (!bShouldSendEvent) return;
	
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
