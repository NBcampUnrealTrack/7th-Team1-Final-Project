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
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Activation")
	ENSAbilityActivationPolicy ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
	
	// 현재 Prediction Key 상태 확인하는 디버그용 함수
	UFUNCTION(BlueprintCallable, Category = "GAS|Debug")
	FString GetCurrentPredictionKeyStatus();
	
	// 현재 Prediction Key가 추가 예측에 유효한지 확인하는 함수
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS|Debug")
	bool IsPredictionKeyValidForMorePrediction() const;
};
