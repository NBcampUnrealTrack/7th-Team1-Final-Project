// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSBaseAttributeSet.h"
#include "NSTurretAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSTurretAttributeSet : public UNSBaseAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DetectionRange, Category = "GAS|Attribute")
	FGameplayAttributeData DetectionRange;
	ATTRIBUTE_ACCESSORS(UNSTurretAttributeSet, DetectionRange);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireRate, Category = "GAS|Attribute")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS(UNSTurretAttributeSet, FireRate);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackRange, Category = "GAS|Attribute")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UNSTurretAttributeSet, AttackRange);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Accuracy, Category = "GAS|Attribute")
	FGameplayAttributeData Accuracy;
	ATTRIBUTE_ACCESSORS(UNSTurretAttributeSet, Accuracy);
	
private:
	UFUNCTION()
	void OnRep_DetectionRange(const FGameplayAttributeData& OldDetectionRange);
	
	UFUNCTION()
	void OnRep_FireRate(const FGameplayAttributeData& OldFireRate);
	
	UFUNCTION()
	void OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange);
	
	UFUNCTION()
	void OnRep_Accuracy(const FGameplayAttributeData& OldAccuracy);
};
