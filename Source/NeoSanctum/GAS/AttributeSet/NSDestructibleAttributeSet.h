// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NSDestructibleAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)           \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 *  Destructible Object를 위한 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSDestructibleAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Destructible")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UNSDestructibleAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Destructible")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UNSDestructibleAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly, Category="Destructible")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UNSDestructibleAttributeSet, Damage)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
