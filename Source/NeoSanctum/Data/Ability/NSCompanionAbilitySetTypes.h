// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "NSCompanionAbilitySetTypes.generated.h"

USTRUCT(BlueprintType)
struct FNSCompanionAbilitySet_GameplayAbility
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly) 
	TSubclassOf<UGameplayAbility> Ability;
	
	UPROPERTY(EditDefaultsOnly) 
	int32 AbilityLevel = 1;
};

USTRUCT(BlueprintType)
struct FNSCompanionAbilitySet_GrantedHandles
{
	GENERATED_BODY()
	
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& SpecHandle);
	void AddGameplayEffectEffectHandle(const FActiveGameplayEffectHandle& EffectHandle);
	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);
	
protected:
	UPROPERTY() 
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	
	UPROPERTY() 
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
};
