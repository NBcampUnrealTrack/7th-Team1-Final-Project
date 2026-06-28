// Copyright 2026 One Team. All rights reserved.

#include "NSHitReactionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueManager.h"

UNSHitReactionComponent::UNSHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSHitReactionComponent::PlayHitReaction(const FNSHitReactionContext& Context) const
{
	if (!DefaultHitCueTag.IsValid())
	{
		return;
	}

	ExecuteHitCue(Context);
}

void UNSHitReactionComponent::ExecuteHitCue(const FNSHitReactionContext& Context) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = Context.InstigatorActor;
	CueParameters.EffectCauser = Context.InstigatorActor;
	CueParameters.Location = Context.HitLocation;
	CueParameters.Normal = Context.HitNormal;
	CueParameters.RawMagnitude = Context.DamageAmount;

	ASC->ExecuteGameplayCue(DefaultHitCueTag, CueParameters);
}
