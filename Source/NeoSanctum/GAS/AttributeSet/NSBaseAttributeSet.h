// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NSBaseAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE(FOnOutOfHealth);

struct FGameplayEffectModCallbackData;

/**
 * 전투 대상이 공통으로 사용하는 AttributeSet
 */
UCLASS()
class NEOSANCTUM_API UNSBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "GAS|Attribute")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Health);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "GAS|Attribute")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, MaxHealth);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "GAS|Attribute")
	FGameplayAttributeData BaseDamage;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, BaseDamage);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "GAS|Attribute")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Defense);
	
	// 실제 CharacterMovement 반영은 Attribute 변경 콜백에서 처리
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "GAS|Attribute")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, MoveSpeed);
	
	// 대쉬 횟수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DashCount, Category = "GAS|Attribute")
	FGameplayAttributeData DashCount;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, DashCount);
	
	// 최대 대쉬 횟수
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxDashCount, Category = "GAS|Attribute")
	FGameplayAttributeData MaxDashCount;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, MaxDashCount);
	
	// 대쉬 횟수 초당 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DashRegenRate, Category = "GAS|Attribute")
	FGameplayAttributeData DashRegenRate;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, DashRegenRate);
	
	// 최종 데미지를 잠시 담는 용이므로 Replicate하지 않음
	UPROPERTY(BlueprintReadOnly, Category = "GAS|Attribute")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UNSBaseAttributeSet, Damage);
	
public:
	FOnOutOfHealth OnOutOfHealth;
	
protected:
	// Health 적용 전 대상별 방어 처리를 수행
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data);
	
private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldBaseDamage);
	
	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldDefense);
	
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
	
	UFUNCTION()
	void OnRep_DashCount(const FGameplayAttributeData& OldDashCount);
	
	UFUNCTION()
	void OnRep_MaxDashCount(const FGameplayAttributeData& OldMaxDashCount);
	
	UFUNCTION()
	void OnRep_DashRegenRate(const FGameplayAttributeData& OldDashRegenRate);
	
private:
	bool bOutOfHealth = false;
};
