// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSBarrierBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNSAbilitySystemComponent;
class UNSBaseAttributeSet;
class UShapeComponent;
class UStaticMeshComponent;
class UNSHitReactionComponent;
class UNSDamageFlashComponent;

UCLASS(Abstract)
class NEOSANCTUM_API ANSBarrierBase : public AActor,
                                      public IAbilitySystemInterface,
                                      public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSBarrierBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(ETeamId::Player));
	}

	void InitializeBarrier(
		APawn* InOwningPawn,
		AController* InOwningController,
		float InRadius,
		float InDuration,
		TSubclassOf<UGameplayEffect> InInitialAttributeEffectClass,
		const TArray<FNSSetByCallerMagnitude>& InSetByCallerMagnitudes
	);

public:
	APawn* GetOwningPawn() const { return OwningPawn; };
	AController* GetOwningController() const { return OwningController; };
	float GetCurrentHealth() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeBarrierCollisionComponent(UShapeComponent* InBarrierCollisionComponent);

	void InitializeAbilityActorInfo();
	void ApplyInitialAttributeEffect();
	virtual void ApplyRadius(float InRadius);
	virtual void ApplyCollisionRadius(float Radius);
	void ApplyDuration(float InDuration);
	void HandleOutOfHealth();
	void DestroyBarrier();
	void ApplyVisualRadius(float Radius);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> ASC;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSBaseAttributeSet> AttributeSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UShapeComponent> BarrierCollisionComponent;
	
	// Barrier 범위 표현과 피격 플래시에 사용하는 Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UStaticMeshComponent> BarrierFlashMeshComponent;
	
	// 실제 Health Damage를 받았을 때 월드 피격 리액션을 재생하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UNSHitReactionComponent> HitReactionComponent;
	
	// 피격 시 머티리얼 플래시를 재생하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UNSDamageFlashComponent> DamageFlashComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float DefaultRadius = 150.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float MinimumRadius = 50.0f;
	
	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;
	
	UPROPERTY(Transient)
	float CurrentRadius = 150.0f;

private:
	UPROPERTY(Transient)
	float CurrentDuration = 0.0f;

	UPROPERTY(Transient)
	TArray<FNSSetByCallerMagnitude> SetByCallerMagnitudes;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(Transient)
	TObjectPtr<AController> OwningController;

	FTimerHandle DurationTimerHandle;

	bool bAbilityActorInfoInitialized = false;
	bool bInitialAttributeEffectApplied = false;
};
