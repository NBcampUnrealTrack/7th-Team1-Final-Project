// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "GA_EngineerBarrier.generated.h"

class ANSBarrier;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FNSBarrierAbilityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	TSubclassOf<ANSBarrier> BarrierClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|Attribute")
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|SetByCaller")
	TArray<FNSSetByCallerFromCombatStat> SetByCallerMappings;
};

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UGA_EngineerBarrier : public UGA_SkillBase
{
	GENERATED_BODY()
	
public:
	UGA_EngineerBarrier();
	
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
	
private:
	bool TryGetBarrierRadius(float& OutBarrierRadius) const;
	bool TryGetBarrierDuration(float& OutBarrierDuration) const;
	void RebuildSetByCallerMagnitudes();
	void SpawnBarrierActor(
		const FGameplayAbilityActorInfo* ActorInfo,
		float BarrierRadius,
		float BarrierDuration,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes
	);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FNSBarrierAbilityConfig BarrierAbilityConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	float MinimumBarrierRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Engineer|Barrier")
	FTransform AttachRelativeTransform = FTransform::Identity;

private:
	UPROPERTY(Transient)
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	UPROPERTY(Transient)
	TObjectPtr<ANSBarrier> ActiveBarrier;
};
