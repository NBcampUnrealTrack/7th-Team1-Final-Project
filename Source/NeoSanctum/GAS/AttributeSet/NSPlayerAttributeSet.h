// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"
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

	// Shield 초당 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldRechargeRate, Category = "GAS|Attribute")
	FGameplayAttributeData ShieldRechargeRate;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, ShieldRechargeRate);

	// Shield 파괴 후 회복 시작까지 대기 시간
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldRechargeCooldown, Category = "GAS|Attribute")
	FGameplayAttributeData ShieldRechargeCooldown;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, ShieldRechargeCooldown);
	
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
	
	// 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Ammo, Category = "GAS|Attribute")
	FGameplayAttributeData Ammo;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, Ammo);
	
	// 최대 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxAmmo, Category = "GAS|Attribute")
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxAmmo);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSkill1Count, Category = "GAS|Attribute")
	FGameplayAttributeData MaxSkill1Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxSkill1Count);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Skill1Count, Category = "GAS|Attribute")
	FGameplayAttributeData Skill1Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, Skill1Count);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSkill2Count, Category = "GAS|Attribute")
	FGameplayAttributeData MaxSkill2Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxSkill2Count);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Skill2Count, Category = "GAS|Attribute")
	FGameplayAttributeData Skill2Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, Skill2Count);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSkill3Count, Category = "GAS|Attribute")
	FGameplayAttributeData MaxSkill3Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, MaxSkill3Count);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Skill3Count, Category = "GAS|Attribute")
	FGameplayAttributeData Skill3Count;
	ATTRIBUTE_ACCESSORS(UNSPlayerAttributeSet, Skill3Count);
protected:
	// Health 적용 전 Shield로 데미지를 흡수
	virtual float HandlePreHealthDamage(float DamageAmount, const FGameplayEffectModCallbackData& Data) override;

	virtual void NotifyHitTakenFeedbackAfterHealthDamage(
		const FGameplayEffectModCallbackData& Data,
		float PreviousHealth) const override;
	
private:
	
	void ResetShieldRechargeFlowOnDamage() const;
	
	// 실제 Notify를 진행해주는 함수
	void NotifyHitTakenFeedback(
		const FGameplayEffectModCallbackData& Data,
		ENSHitTakenFeedbackType FeedbackType,
		float DamageAmount) const;

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);
	
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	void OnRep_ShieldRechargeRate(const FGameplayAttributeData& OldShieldRechargeRate);

	UFUNCTION()
	void OnRep_ShieldRechargeCooldown(const FGameplayAttributeData& OldShieldRechargeCooldown);
	
	UFUNCTION()
	void OnRep_DashCount(const FGameplayAttributeData& OldDashCount);
	
	UFUNCTION()
	void OnRep_MaxDashCount(const FGameplayAttributeData& OldMaxDashCount);
	
	UFUNCTION()
	void OnRep_DashRegenRate(const FGameplayAttributeData& OldDashRegenRate);
	
	UFUNCTION()
	void OnRep_Ammo(const FGameplayAttributeData& OldAmmo);
	
	UFUNCTION()
	void OnRep_MaxAmmo(const FGameplayAttributeData& OldMaxAmmo);
	
	UFUNCTION()
	void OnRep_Skill1Count(const FGameplayAttributeData& OldSkill1Count);
	
	UFUNCTION()
	void OnRep_MaxSkill1Count(const FGameplayAttributeData& OldMaxSkill1Count);
	
	UFUNCTION()
	void OnRep_Skill2Count(const FGameplayAttributeData& OldSkill2Count);
	
	UFUNCTION()
	void OnRep_MaxSkill2Count(const FGameplayAttributeData& OldMaxSkill2Count);
	
	UFUNCTION()
	void OnRep_Skill3Count(const FGameplayAttributeData& OldSkill3Count);
	
	UFUNCTION()
	void OnRep_MaxSkill3Count(const FGameplayAttributeData& OldMaxSkill3Count);
};
