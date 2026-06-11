// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NSCompanionData.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;
class UGameplayAbility;
/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSCompanionData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UAttributeSet>> GrantedAttributeSet;

};
