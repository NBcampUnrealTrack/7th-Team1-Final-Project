// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "NeoSanctum/Data/Combat/NSCombatStatTypes.h"
#include "NeoSanctum/Type/NSTeamTypes.h"
#include "NSBarrier.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNSAbilitySystemComponent;
class UNSBaseAttributeSet;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNSHitReactionComponent;
class UNSDamageFlashComponent;

UCLASS()
class NEOSANCTUM_API ANSBarrier : public AActor,
                                 public IAbilitySystemInterface,
                                 public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ANSBarrier();

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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeAbilityActorInfo();
	void ApplyInitialAttributeEffect();
	void ApplyRadius(float InRadius);
	void ApplyDuration(float InDuration);
	void HandleOutOfHealth();
	void DestroyBarrier();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> ASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSBaseAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<USphereComponent> BarrierCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UNiagaraComponent> BarrierNiagaraComponent;

	// 피격 플래시 표현에 사용하는 투명 Sphere Mesh
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

	// Flash용 Sphere Mesh의 기본 반지름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float FlashMeshBaseRadius = 60.0f;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> InitialAttributeEffectClass;

private:
	UPROPERTY(Transient)
	float CurrentRadius = 150.0f;

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
