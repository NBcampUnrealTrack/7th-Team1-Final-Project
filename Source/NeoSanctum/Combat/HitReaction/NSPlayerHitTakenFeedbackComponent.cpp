// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerHitTakenFeedbackComponent.h"

#include "GameFramework/Pawn.h"

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
	
	// TODO : 여기에 래핑된 피드백들 전부 들어올 예정
}

bool UNSPlayerHitTakenFeedbackComponent::ShouldPlayLocalFeedback() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
