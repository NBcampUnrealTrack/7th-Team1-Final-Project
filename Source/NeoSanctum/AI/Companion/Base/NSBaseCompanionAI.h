// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NSBaseCompanionAI.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class ANSDroneAIController;


UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public APawn
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "AI|CachedData")
	TObjectPtr<ANSDroneAIController> CachedAIController;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeSet> AttributeSet;
	
public:
	FORCEINLINE UAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	FORCEINLINE UAttributeSet* GetCompanionAttributeSet() const {return AttributeSet;}
};
