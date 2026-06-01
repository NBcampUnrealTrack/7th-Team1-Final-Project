// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSPlayerAttributeSet.generated.h"

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
	
	// 대쉬 횟수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DashCount, Category = "GAS|Attribute")
	FGameplayAttributeData DashCount;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, DashCount);
	
	// 최대 대쉬 횟수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxDashCount, Category = "GAS|Attribute")
	FGameplayAttributeData MaxDashCount;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxDashCount);
	
	// 대쉬 횟수 초당 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DashRegenRate, Category = "GAS|Attribute")
	FGameplayAttributeData DashRegenRate;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, DashRegenRate);
	
protected:
	// Health 적용 전 Shield로 데미지를 흡수
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data) override;
	
private:
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);
	
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
	
	UFUNCTION()
	void OnRep_DashCount(const FGameplayAttributeData& OldDashCount);
	
	UFUNCTION()
	void OnRep_MaxDashCount(const FGameplayAttributeData& OldMaxDashCount);
	
	UFUNCTION()
	void OnRep_DashRegenRate(const FGameplayAttributeData& OldDashRegenRate);
};
