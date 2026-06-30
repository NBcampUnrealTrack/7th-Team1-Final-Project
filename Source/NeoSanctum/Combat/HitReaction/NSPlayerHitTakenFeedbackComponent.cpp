// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerHitTakenFeedbackComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UNSPlayerHitTakenFeedbackComponent::UNSPlayerHitTakenFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerHitTakenFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	TryBindShieldRechargingTagEvent();
}

void UNSPlayerHitTakenFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindShieldRechargingTagEvent();

	Super::EndPlay(EndPlayReason);
}

void UNSPlayerHitTakenFeedbackComponent::HandleHitTakenFeedback(
	const FNSHitTakenFeedbackContext& Context)
{
	if (!ShouldPlayLocalFeedback())
	{
		return;
	}

	TryBindShieldRechargingTagEvent();

	FNSHitTakenFeedbackMessage Message;
	Message.Context = Context;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_HitTakenFeedback,
		Message);
}

void UNSPlayerHitTakenFeedbackComponent::TryBindShieldRechargingTagEvent()
{
	if (!ShouldPlayLocalFeedback() || CachedASC.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	CachedASC = ASC;
	ShieldRechargingTagDelegateHandle =
		ASC->RegisterGameplayTagEvent(
			NSGameplayTags::State_Shield_Recharging,
			EGameplayTagEventType::NewOrRemoved).AddUObject(
				this,
				&ThisClass::HandleShieldRechargingTagChanged);

	BroadcastHitTakenFeedbackState(
		ENSHitTakenFeedbackStateType::ShieldRecharging,
		ASC->HasMatchingGameplayTag(NSGameplayTags::State_Shield_Recharging));
}

void UNSPlayerHitTakenFeedbackComponent::UnbindShieldRechargingTagEvent()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RegisterGameplayTagEvent(
			NSGameplayTags::State_Shield_Recharging,
			EGameplayTagEventType::NewOrRemoved).Remove(ShieldRechargingTagDelegateHandle);
	}

	CachedASC.Reset();
	ShieldRechargingTagDelegateHandle.Reset();
}

void UNSPlayerHitTakenFeedbackComponent::HandleShieldRechargingTagChanged(
	const FGameplayTag Tag,
	const int32 NewCount)
{
	BroadcastHitTakenFeedbackState(
		ENSHitTakenFeedbackStateType::ShieldRecharging,
		NewCount > 0);
}

void UNSPlayerHitTakenFeedbackComponent::BroadcastHitTakenFeedbackState(
	const ENSHitTakenFeedbackStateType StateType,
	const bool bActive) const
{
	if (!ShouldPlayLocalFeedback())
	{
		return;
	}

	FNSHitTakenFeedbackStateMessage Message;
	Message.StateType = StateType;
	Message.bActive = bActive;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_HitTakenFeedback_State,
		Message);
}

bool UNSPlayerHitTakenFeedbackComponent::ShouldPlayLocalFeedback() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
