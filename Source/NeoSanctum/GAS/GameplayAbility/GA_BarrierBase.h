// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "GA_BarrierBase.generated.h"

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

UCLASS(Abstract)
class NEOSANCTUM_API UGA_BarrierBase : public UGA_SkillBase
{
	GENERATED_BODY()

protected:
	bool HasValidBarrierConfig() const;
	bool TryGetBarrierRadius(float& OutBarrierRadius) const;
	bool TryGetBarrierDuration(float& OutBarrierDuration) const;
	void RebuildSetByCallerMagnitudes();

	ANSBarrier* SpawnBarrierActor(
		const FGameplayAbilityActorInfo* ActorInfo,
		float BarrierRadius,
		float BarrierDuration,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes
	);

	const TArray<FNSSetByCallerMagnitude>& GetSetByCallerMagnitudes() const { return SetByCallerMagnitudes; }
	ANSBarrier* GetActiveBarrier() const { return ActiveBarrier; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Barrier")
	FNSBarrierAbilityConfig BarrierAbilityConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Barrier")
	float MinimumBarrierRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Barrier")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Barrier")
	FTransform AttachRelativeTransform = FTransform::Identity;

private:
	UPROPERTY(Transient)
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	UPROPERTY(Transient)
	TObjectPtr<ANSBarrier> ActiveBarrier;
};
