// Copyright 2026 One Team. All rights reserved.

#include "NSBossModeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"

UNSBossModeComponent::UNSBossModeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNSBossModeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNSBossModeComponent, CurrentModeTag);
}

void UNSBossModeComponent::InitializeMode()
{
	if (IsOwnerAuthority() && !CurrentModeTag.IsValid() && DefaultModeTag.IsValid())
	{
		SetMode(DefaultModeTag);
		return;
	}

	SyncASCModeTag(CurrentModeTag);
}

bool UNSBossModeComponent::SetMode(FGameplayTag NewModeTag)
{
	if (!IsOwnerAuthority())
	{
		return false;
	}

	if (!NewModeTag.IsValid())
	{
		ClearMode();
		return true;
	}

	if (CurrentModeTag == NewModeTag)
	{
		return true;
	}

	const FGameplayTag PreviousModeTag = CurrentModeTag;
	CurrentModeTag = NewModeTag;

	HandleModeChanged(PreviousModeTag, CurrentModeTag);

	return true;
}

void UNSBossModeComponent::ClearMode()
{
	if (!IsOwnerAuthority() || !CurrentModeTag.IsValid())
	{
		return;
	}

	const FGameplayTag PreviousModeTag = CurrentModeTag;
	CurrentModeTag = FGameplayTag();

	HandleModeChanged(PreviousModeTag, CurrentModeTag);
}

bool UNSBossModeComponent::IsInMode(FGameplayTag ModeTag) const
{
	return CurrentModeTag.IsValid() &&
		ModeTag.IsValid() &&
		CurrentModeTag.MatchesTag(ModeTag);
}

void UNSBossModeComponent::OnRep_CurrentModeTag(FGameplayTag PreviousModeTag)
{
	HandleModeChanged(PreviousModeTag, CurrentModeTag);
}

void UNSBossModeComponent::HandleModeChanged(
	FGameplayTag PreviousModeTag,
	FGameplayTag NewModeTag)
{
	SyncASCModeTag(NewModeTag);
	OnBossModeChanged.Broadcast(PreviousModeTag, NewModeTag);
}

void UNSBossModeComponent::SyncASCModeTag(FGameplayTag NewModeTag)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	if (AppliedModeTag.IsValid() && AppliedModeTag != NewModeTag)
	{
		ASC->RemoveLooseGameplayTag(AppliedModeTag);
		AppliedModeTag = FGameplayTag();
	}

	if (NewModeTag.IsValid() && AppliedModeTag != NewModeTag)
	{
		ASC->AddLooseGameplayTag(NewModeTag);
		AppliedModeTag = NewModeTag;
	}
}

bool UNSBossModeComponent::IsOwnerAuthority() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor && OwnerActor->HasAuthority();
}

UAbilitySystemComponent* UNSBossModeComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}
