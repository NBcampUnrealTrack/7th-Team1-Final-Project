// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NSBaseCompanionAI.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;
class UFloatingPawnMovement;
class USceneComponent;

UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFloatingPawnMovement> MovementComp;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RootScene;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystemComponent")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystemComponent")
	TObjectPtr<UAttributeSet> AttributeSet;
	
public:
	FORCEINLINE UAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	FORCEINLINE UAttributeSet* GetCompanionAttributeSet() const {return AttributeSet;}
};
