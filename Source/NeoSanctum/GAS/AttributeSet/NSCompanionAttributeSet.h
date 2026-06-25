// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NSBaseAttributeSet.h"
#include "NSCompanionAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSCompanionAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing= OnRep_AttackDamage , Category="GAS|DroneAttributes")
	FGameplayAttributeData AttackDamage;
	ATTRIBUTE_ACCESSORS(UNSCompanionAttributeSet, AttackDamage);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing= OnRep_FireRate, Category="GAS|DroneAttributes")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS(UNSCompanionAttributeSet, FireRate);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing= OnRep_ProjectileSpeed, Category="GAS|DroneAttributes")
	FGameplayAttributeData ProjectileSpeed;
	ATTRIBUTE_ACCESSORS(UNSCompanionAttributeSet, ProjectileSpeed);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing= OnRep_AttackRange, Category="GAS|DroneAttributes")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UNSCompanionAttributeSet, AttackRange);
	
protected:
	UFUNCTION()
	void OnRep_AttackDamage(const FGameplayAttributeData& OldAttackDamage);
	
	UFUNCTION()
	void OnRep_FireRate(const FGameplayAttributeData& OldFireRate);
	
	UFUNCTION()
	void OnRep_ProjectileSpeed(const FGameplayAttributeData& OldProjectileSpeed);
	
	UFUNCTION()
	void OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
