// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSBarrier.generated.h"

class USphereComponent;
class UNiagaraComponent;

UCLASS()
class NEOSANCTUM_API ANSBarrier : public AActor
{
	GENERATED_BODY()

public:
	ANSBarrier();

	void InitializeBarrier(
		APawn* InOwningPawn,
		AController* InOwningController,
		float InRadius
	);
	
public:
	APawn* GetOwningPawn() const { return OwningPawn; };
	AController* GetOwningController() const { return OwningController; };

protected:
	virtual void BeginPlay() override;

private:
	void ApplyRadius(float InRadius);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<USphereComponent> BarrierCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier|Components")
	TObjectPtr<UNiagaraComponent> BarrierNiagaraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float DefaultRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier")
	float MinimumRadius = 50.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(Transient)
	TObjectPtr<AController> OwningController;
};
