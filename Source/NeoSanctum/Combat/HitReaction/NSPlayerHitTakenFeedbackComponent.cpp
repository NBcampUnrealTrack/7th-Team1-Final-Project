// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerHitTakenFeedbackComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
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
	TryBindVitalsAttributeEvents();
}

void UNSPlayerHitTakenFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindVitalsAttributeEvents();
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
	TryBindVitalsAttributeEvents();

	FNSHitTakenFeedbackMessage Message;
	Message.Context = Context;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_HitTakenFeedback,
		Message);
}

void UNSPlayerHitTakenFeedbackComponent::TryBindVitalsAttributeEvents()
{
	if (!ShouldPlayLocalFeedback() || bVitalsAttributeEventsBound)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	}

	if (!ASC)
	{
		return;
	}

	CachedASC = ASC;
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetHealthAttribute()).AddUObject(
			this,
			&ThisClass::HandleVitalsAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetMaxHealthAttribute()).AddUObject(
			this,
			&ThisClass::HandleVitalsAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetShieldAttribute()).AddUObject(
			this,
			&ThisClass::HandleVitalsAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetMaxShieldAttribute()).AddUObject(
			this,
			&ThisClass::HandleVitalsAttributeChanged);

	bVitalsAttributeEventsBound = true;
	BroadcastHitTakenFeedbackVitals();
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

void UNSPlayerHitTakenFeedbackComponent::UnbindVitalsAttributeEvents()
{
	if (!bVitalsAttributeEventsBound)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetShieldAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetMaxShieldAttribute()).RemoveAll(this);
	}

	bVitalsAttributeEventsBound = false;
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

void UNSPlayerHitTakenFeedbackComponent::HandleVitalsAttributeChanged(
	const FOnAttributeChangeData& Data)
{
	BroadcastHitTakenFeedbackVitals();
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

void UNSPlayerHitTakenFeedbackComponent::BroadcastHitTakenFeedbackVitals() const
{
	if (!ShouldPlayLocalFeedback())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	const float MaxHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	const float Health = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float MaxShield = ASC->GetNumericAttribute(UNSPlayerAttributeSet::GetMaxShieldAttribute());
	const float Shield = ASC->GetNumericAttribute(UNSPlayerAttributeSet::GetShieldAttribute());

	FNSHitTakenFeedbackVitalsMessage Message;
	Message.HealthRatio = MaxHealth > 0.0f
		                      ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f)
		                      : 0.0f;
	Message.ShieldRatio = MaxShield > 0.0f
		                      ? FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f)
		                      : 0.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_HitTakenFeedback_Vitals,
		Message);
}

bool UNSPlayerHitTakenFeedbackComponent::ShouldPlayLocalFeedback() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
