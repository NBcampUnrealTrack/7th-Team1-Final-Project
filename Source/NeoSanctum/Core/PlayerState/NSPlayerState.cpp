// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerState.h"

#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"

ANSPlayerState::ANSPlayerState()
{
	// PlayerState의 기본 Frequency는 1Hz(매우 낮음)
	SetNetUpdateFrequency(100.0f);
	
	AbilitySystemComponent = CreateDefaultSubobject<UNSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	PlayerAttributeSet = CreateDefaultSubobject<UNSPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
}

UAbilitySystemComponent* ANSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UNSPlayerAttributeSet* ANSPlayerState::GetPlayerAttributeSet() const
{
	return PlayerAttributeSet;
}
