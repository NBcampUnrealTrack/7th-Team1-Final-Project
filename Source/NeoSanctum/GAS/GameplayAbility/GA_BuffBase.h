// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "GA_BuffBase.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ENSBuffTargetType : uint8
{
	Self,
	SingleTarget,
	Radius
};

USTRUCT(BlueprintType)
struct FNSBuffTargetFilter
{
	GENERATED_BODY()

	// 자신 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeSelf = true;

	// 자신이 소환한 Turret 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeOwnedTurrets = false;

	// 다른 플레이어 포함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Target")
	bool bIncludeOtherPlayers = false;
};

USTRUCT(BlueprintType)
struct FNSBuffEffectEntry
{
	GENERATED_BODY()

	// 적용 대상 Actor 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Effect")
	TSubclassOf<AActor> TargetActorClass;

	// 대상에게 적용할 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|Effect")
	TSubclassOf<UGameplayEffect> EffectClass;
};

/**
 * 버프를 줄 수 있는 Ability
 * 
 * 어떤 대상에게 어떤 GE를 적용할 것인지를 결정하는 간단한 Ability
 */
UCLASS()
class NEOSANCTUM_API UGA_BuffBase : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_BuffBase();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	// 버프 대상 수집 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Target")
	ENSBuffTargetType TargetType = ENSBuffTargetType::Self;

	// 버프 대상 필터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Target")
	FNSBuffTargetFilter TargetFilter;

	// 대상 타입별 적용 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|Effect")
	TArray<FNSBuffEffectEntry> BuffEffectEntries;

	// CombatStat 값을 GameplayEffect SetByCaller 값으로 넘기기 위한 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|SetByCaller")
	TArray<FNSSetByCallerFromCombatStat> SetByCallerMappings;

	// CombatStat 조회에 사용할 Ability 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Buff|SetByCaller")
	FGameplayTag CombatStatAbilityTag;
};
