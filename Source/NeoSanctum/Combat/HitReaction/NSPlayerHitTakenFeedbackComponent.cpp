// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerHitTakenFeedbackComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"

UNSPlayerHitTakenFeedbackComponent::UNSPlayerHitTakenFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerHitTakenFeedbackComponent::HandleHitTakenFeedback(
	const FNSHitTakenFeedbackContext& Context)
{
	if (!ShouldPlayLocalFeedback())
	{
		return;
	}

	FNSHitTakenFeedbackMessage Message;
	Message.Context = Context;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_HitTakenFeedback,
		Message);
}

bool UNSPlayerHitTakenFeedbackComponent::ShouldPlayLocalFeedback() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
