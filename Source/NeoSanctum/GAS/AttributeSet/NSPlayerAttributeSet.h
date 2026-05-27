// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSPlayerAttributeSet.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnOutOfHealth);

/**
 * 플레이어 전용 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "GAS|Attribute")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, Shield);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "GAS|Attribute")
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxShield);

public:
	FOnOutOfHealth OnOutOfHealth;
	
protected:
	// Health 적용 전 Shield로 데미지를 흡수
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data) override;
	
private:
	bool bOutOfHealth = false;
	
private:
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);
	
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
};
