// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_SkillBase.generated.h"

class ANSPlayerState;
class UNSAbilitySystemComponent;

UENUM(BlueprintType)
enum class ENSAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive
};

/**
 * 플레이어 스킬 공통 베이스
 */
UCLASS(Abstract)
class NEOSANCTUM_API UGA_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SkillBase();
	
	ENSAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	// Cost/Cooldown Commit 성공 시점에 스킬 충전 회복을 시작하도록 함
	virtual bool CommitAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) override;

protected:
	UNSAbilitySystemComponent* GetNSAbilitySystemComponent() const;
	
	ANSPlayerState* GetNSPlayerState() const;
	
protected:
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
	
	float GetFinalAbilityStatOrDefault(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float DefaultValue
	) const;

	float GetBaseAbilityStatOrDefault(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& StatTag,
		float DefaultValue
	) const;
	
	// 스킬 데이터의 쿨다운 태그에 따른 Value 찾아오기 : FinalAbilityStat 기준으로 적용
   	float GetCooldownStatOrDefault() const;
	
	// 조건에 맞는 경우에 스킬의 재충전(Cooldown)을 시작 
	void StartRechargeIfNeeded();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Activation")
	ENSAbilityActivationPolicy ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	
	// 현재 Prediction Key 상태 확인하는 디버그용 함수
	UFUNCTION(BlueprintCallable, Category = "GAS|Debug")
	FString GetCurrentPredictionKeyStatus();
	
	// 현재 Prediction Key가 추가 예측에 유효한지 확인하는 함수
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS|Debug")
	bool IsPredictionKeyValidForMorePrediction() const;
	
protected:
	// 스킬 어빌리티 태그
	// TODO: 스킬에 산재되어있는 Ability Tag를 여기로 통일하는 리팩토링
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill")
	FGameplayTag SkillAbilityTag;
	
	// 스킬 슬롯 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill")
	FGameplayTag SkillSlotTag;
	
	// Default Cooldown Fallback : CombatStat.Cooldown 태그로 Value를 매핑해뒀다면 적용되지 않음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill")
	float DefaultCooldown = 5.0f;
};
