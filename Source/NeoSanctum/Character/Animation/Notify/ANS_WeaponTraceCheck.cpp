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

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld()) return;
	
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	FGameplayEventData EventData;
	EventData.Instigator = Owner;
	EventData.Target = Owner;

	for (const FGameplayTag& Tag : EventTags)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Tag, EventData);
	}
}
