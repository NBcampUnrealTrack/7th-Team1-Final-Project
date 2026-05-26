// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NSBaseCompanionAI.generated.h"

class UAIPerceptionComponent;
class UFloatingPawnMovement;
class USceneComponent;
class UCapsuleComponent;

UCLASS()
class NEOSANCTUM_API ANSBaseCompanionAI : public ACharacter
{
	GENERATED_BODY()

public:
	ANSBaseCompanionAI();

protected:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeSet> AttributeSet;
	
public:
	FORCEINLINE UAbilitySystemComponent* GetCompanionAbilitySystemComponent() const {return AbilitySystemComponent;}
	
	FORCEINLINE UAttributeSet* GetCompanionAttributeSet() const {return AttributeSet;}
};
