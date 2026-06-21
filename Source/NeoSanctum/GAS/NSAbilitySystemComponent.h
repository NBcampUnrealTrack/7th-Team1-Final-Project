// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NSAbilitySystemComponent.generated.h"

class UGameplayEffect;

/**
 * InputTag 기반 Ability 입력 처리 담당을 위한 ASC.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	
	// 입력 이벤트를 한 프레임 단위로 모아서 처리
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	
	// 입력 상태를 강제로 비울 때 사용
	void ClearAbilityInput();
	
	// 스킬 슬롯별 재충전 진입점
	void StartSkillRecharge(const FGameplayTag& SkillSlotTag, float Cooldown);

public:
	bool TryGetBaseAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool TryGetFinalAbilityStat(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float& OutValue
	) const;
	
	bool IsAbilityStatModifiable(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag
	) const;

	// CombatStatComponent에 TemporaryModifier 등록
	FGuid AddTemporaryCombatStatModifier(
		const FGameplayTag& TargetAbilityTag,
		const FGameplayTag& StatTag,
		ENSCombatStatModifierOperation Operation,
		float Value,
		float Duration
	) const;

	// CombatStatComponent의 TemporaryModifier 제거
	void RemoveTemporaryCombatStatModifier(FGuid Handle) const;
	
private:
	// 해당 슬롯의 스킬이 이미 재충전 중인지 판단
	bool IsSkillRechargeActive(const FGameplayTag& SkillSlotTag) const;
	// 해당 슬롯의 스킬이 이미 MaxCount인지 판단
	bool IsSkillCountFull(const FGameplayTag& SkillSlotTag) const;
	// 슬롯에 해당하는 Recharge GE를 찾아 반환하는 함수
	TSubclassOf<UGameplayEffect> GetRechargeGEClassForSlot(const FGameplayTag& SkillSlotTag) const;
	// 슬롯에 해당하는 SetByCaller Effect Tag 반환
	FGameplayTag GetRechargeEffectTagForSlot(const FGameplayTag& SkillSlotTag) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill1RechargeGEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill2RechargeGEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Recharge")
	TSubclassOf<UGameplayEffect> Skill3RechargeGEClass;
	
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
