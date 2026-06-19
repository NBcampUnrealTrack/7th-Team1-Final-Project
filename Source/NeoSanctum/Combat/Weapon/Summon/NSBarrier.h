// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "NSBarrier.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNSAbilitySystemComponent;
class UNSBaseAttributeSet;
class USphereComponent;
class UNiagaraComponent;

UCLASS()
class NEOSANCTUM_API ANSBarrier : public AActor,
                                 public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSBarrier();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void InitializeBarrier(
		APawn* InOwningPawn,
		AController* InOwningController,
		float InRadius,
		float InMaxHealth
	);
	
public:
	APawn* GetOwningPawn() const { return OwningPawn; };
	AController* GetOwningController() const { return OwningController; };

protected:
	virtual void BeginPlay() override;

private:
	void InitializeAbilityActorInfo();
	void ApplyInitialAttributeEffect();
	void ApplyRadius(float InRadius);
	void HandleOutOfHealth();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> ASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSBaseAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<USphereComponent> BarrierCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UNiagaraComponent> BarrierNiagaraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float DefaultRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float MinimumRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|Attribute")
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;

private:
	UPROPERTY(Transient)
	float CurrentRadius = 150.0f;

	UPROPERTY(Transient)
	float CurrentMaxHealth = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(Transient)
	TObjectPtr<AController> OwningController;

	bool bAbilityActorInfoInitialized = false;
	bool bInitialAttributeEffectApplied = false;
};
